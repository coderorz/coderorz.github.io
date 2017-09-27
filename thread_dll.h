#pragma once

#ifdef WIN32
//////////windows///////////////
#include <iostream>		//std::nothrow
#include <windows.h>	//for HANDLE
#include <process.h>	//for _beginthreadex
#pragma comment(lib, "Winmm.lib")
/////////////////////////
#else
//////////linux///////////////
#include <pthread.h>
/////////////////////////
#endif



#include <map>

//////////////////////////////////public////////////////////////////////////////
typedef void* (*pvoid_proc_pvoid)(void*);

#ifdef WIN32
#define cb_sleep(t) Sleep(t);
#else
#define cb_sleep(t) usleep(t * 1000);
#endif
//////////////////////////////////public////////////////////////////////////////

//////////////////////////////////lock//////////////////////////////////////////
#ifdef WIN32
#define cb_lockexchange(k, v) InterlockedExchange(&k, v)
#define cb_lockcompareexchange(k, v, c) InterlockedCompareExchange(&k, v, c)
#define cb_lockexadd(k, v) InterlockedExchangeAdd(&k,  v)
#define cb_lockexsub(k, v) InterlockedExchangeAdd(&k, -v)
#else
#define cb_lockexchange(k, v) __sync_val_compare_and_swap(&k, k, v);
#define cb_lockcompareexchange(k, v, c) __sync_val_compare_and_swap(&k, c, v)
#define cb_lockexadd(k, v) __sync_fetch_and_add(&k, v);
#define cb_lockexsub(k, v) __sync_fetch_and_sub(&k, v);
#endif
//////////////////////////////////lock//////////////////////////////////////////

//////////////////////////////////thread////////////////////////////////////////
#ifdef WIN32
#define cb_thread_fd HANDLE
#define cb_create_thread(thread_fd, proc, params) ((thread_fd) = \
	(cb_thread_fd)_beginthreadex(0, 0, (proc), (params), 0, 0))
#define cb_thread_fail(thread_fd) (0 == (thread_fd))
#define cb_close_thread_fd(thread_fd) CloseHandle(thread_fd)
#else
#define cb_thread_fd pthread_t
#define cb_create_thread(thread_fd, proc, params) pthread_attr_t cb__attr;\
	pthread_attr_init(&cb__attr);\
	pthread_attr_setdetachstate(&cb__attr, 1);\
	pthread_create(&thread_fd, &cb__attr, proc, (void*)params)
#define cb_thread_fail(thread_fd) (0 != (thread_fd))
#define cb_close_thread_fd(thread_fd)
#endif
//////////////////////////////////thread////////////////////////////////////////

namespace cb_space_memorypool{
	class mem_pool
	{
		struct mem_node
		{
		public:
			mem_node():m_nlock(0), m_pmemhead(0), m_ntlock(0), m_pmemt(0), m_ielemsize(0), m_pblock(0), m_pnextnode(0){}
		public:
			volatile long m_nlock;			//连续内存单元锁
			void* m_pmemhead;				//连续内存单元的第一个地址,每个单元头sizeof(void*)个字节内容指向下一个内存单元的地址
			volatile long m_ntlock;			//临时内存单元锁
			void* m_pmemt;					//临时存储一个变量,可提高效率
			unsigned int m_ielemsize;		//每个内存块的size
			void* m_pblock;					//内存块地址,后sizeof(void*)个字节内容指向下一个内存块的地址,但是内存块都插入到m_pmemhead
			mem_node* m_pnextnode;	//指向下个node
		public:
			void* new_mem(int iflag = 1)
			{
				//temp memory
				if(m_pmemt)
				{
					if(cb_lockcompareexchange(m_ntlock, 1, 0) == 0)
					{
						if(m_pmemt)
						{
							void* p = m_pmemt;
							m_pmemt = 0;
							cb_lockexchange(m_ntlock, 0);
							return p;
						}
						cb_lockexchange(m_ntlock, 0);
					}
				}
				//series memory
				if(m_pmemhead)
				{
					if(cb_lockcompareexchange(m_nlock, 1, 0) == 0)
					{
						if(m_pmemhead){
							void* p = m_pmemhead;
							memcpy(&m_pmemhead, p, sizeof(void*));
							cb_lockexchange(m_nlock, 0);
							return p;
						}
						cb_lockexchange(m_nlock, 0);
						if(iflag & 1)
						{
							if(m_pnextnode){
								return m_pnextnode->new_mem(iflag);
							}
							else{
								return new_elem(this);
							}
						}
					}
					else{
						if(iflag & 1)
						{
							if(m_pnextnode){
								return m_pnextnode->new_mem(iflag);
							}
							else{
								return new_elem(this);
							}
						}
						else{
							while(cb_lockcompareexchange(m_nlock, 1, 0) == 1);
							if(m_pmemhead){
								void* p = m_pmemhead;
								memcpy(&m_pmemhead, p, sizeof(void*));
								cb_lockexchange(m_nlock, 0);
								return p;
							}
							cb_lockexchange(m_nlock, 0);
						}
					}
				}
				//new memory block
				void* ptail = 0;
				void* phead = new_elem(this, 1000, true, &ptail);
				if(!phead || !ptail){
					return 0;
				}
				while(cb_lockcompareexchange(m_nlock, 1, 0) == 1);
				if(m_pmemhead){
					memcpy(ptail, &m_pmemhead, sizeof(void*));
				}
				m_pmemhead = phead;
				void* p = m_pmemhead;
				memcpy(&m_pmemhead, p, sizeof(void*));
				cb_lockexchange(m_nlock, 0);
				return p;
			}
			void  del_mem(void* p)
			{
				if(!m_pmemt && (cb_lockcompareexchange(m_ntlock, 1, 0) == 0))
				{
					if(!m_pmemt)
					{
						m_pmemt = p;
						cb_lockexchange(m_ntlock, 0);
						return ;
					}
					cb_lockexchange(m_ntlock, 0);
				}
				//连续内存单元
				while(cb_lockcompareexchange(m_nlock, 1, 0) == 1);
				memcpy(p, &m_pmemhead, sizeof(void*));
				m_pmemhead = p;
				cb_lockexchange(m_nlock, 0);
				return ;
			}
			void* new_elem(mem_node* pnode, unsigned int ielemcount = 1024, bool bnewelem = false, void** ptail = 0)
			{
				//bnewelem = false and ptail = 0, or ptail can not be 0
				volatile static long lock = 0;
				unsigned int ielemsize = pnode->m_ielemsize;
				while(cb_lockcompareexchange(lock, 1, 0) == 1);
				if(!bnewelem)
				{
					char* p = new(std::nothrow)char[ielemsize + sizeof(void*)];
					if(p)
					{
						memset(p, 0, sizeof(mem_node*));
						p += sizeof(mem_node*);//0 flag for system delete
					}
					cb_lockexchange(lock, 0);
					return p;
				}
				unsigned int size = ielemsize * ielemcount;
				char* p = new(std::nothrow)char[size];
				if(!p){
					cb_lockexchange(lock, 0);
					return 0;
				}
				char* pend = p + size;
				memset(p, 0, size);
				if(pnode->m_pblock){
					memcpy(p, &pnode->m_pblock, sizeof(void*));
				}
				pnode->m_pblock = p;
				p += sizeof(void*);
				void* phead = p + sizeof(mem_node*);
				while(1)
				{
					//node add
					memcpy(p, &pnode, sizeof(mem_node*));
					p += sizeof(mem_node*);
					*ptail = p;//set last elem for insert list
					//elemnext
					char* pnext = p + ielemsize + sizeof(mem_node*);
					if(pnext + ielemsize > pend){
						break;
					}
					memcpy(p, &pnext, sizeof(void*));
					//nextnode
					p += ielemsize;
				}
				cb_lockexchange(lock, 0);
				return phead;
			}
		};
	public:
		mem_pool(void)
			:m_pmemorydelhead(0), m_p__16(0), m_p__32(0), m_p__64(0), m_p_128(0), m_p_256(0), m_p_512(0), m_p1024(0), m_p2048(0), m_p4096(0), m_p8192(0)
		{
			std::map<unsigned int, unsigned int> m_bmap;
			m_bmap[16]	= 4096;	m_bmap[32]	= 4096;	m_bmap[64]	 = 4096; m_bmap[128]  = 2048; m_bmap[256] = 2048;
			m_bmap[512]	= 1024;	m_bmap[1024]= 1024;	m_bmap[2048] = 1024; m_bmap[4096] = 1024; m_bmap[8192]= 1024;
			//m_bmap[16]	= 100;	m_bmap[32]	= 100;	m_bmap[64]	 = 100; m_bmap[128]  = 100; m_bmap[256] = 100;
			//m_bmap[512]	= 100;	m_bmap[1024]= 100;	m_bmap[2048] = 100; m_bmap[4096] = 100; m_bmap[8192]= 100;
			unsigned int arr[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};
			mem_node* pnodehead = 0;
			for(int i = sizeof(arr)/sizeof(arr[0]) - 1; i >= 0; --i)
			{
				mem_node* pnode = new_node(arr[i], m_bmap[arr[i]]);
				if(!pnode){
					continue;
				}
				if(pnodehead){
					pnode->m_pnextnode = pnodehead;
				}
				pnodehead = pnode;
				switch(i){
				case 0:	m_p__16	= pnode;break;
				case 1:	m_p__32	= pnode;break;
				case 2:	m_p__64	= pnode;break;
				case 3:	m_p_128	= pnode;break;
				case 4:	m_p_256	= pnode;break;
				case 5:	m_p_512	= pnode;break;
				case 6:	m_p1024	= pnode;break;
				case 7:	m_p2048	= pnode;break;
				case 8:	m_p4096	= pnode;break;
				case 9:	m_p8192	= pnode;break;
				default:break;
				}
			}
			m_pmemorydelhead = pnodehead;
		}
		virtual ~mem_pool(void)
		{
			m_p__16 = 0; m_p__32 = 0; m_p__64 = 0; m_p_128 = 0; m_p_256 = 0;
			m_p_512 = 0; m_p1024 = 0; m_p2048 = 0; m_p4096 = 0; m_p8192 = 0;
			mem_node* phead = m_pmemorydelhead;
			while(phead)
			{
				char* p = (char*)phead->m_pblock;
				while(p){
					char* pnext = 0;
					memcpy(&pnext, p, sizeof(char*));
					delete p, p = pnext;
				}
				char* pdel = (char*)phead;
				phead = phead->m_pnextnode;
				delete pdel, pdel = 0;
			}
			m_pmemorydelhead = 0;
		}
	public:
		void* mem_pool_new(unsigned int n, int iflag = 1)
		{
			//n = cb_alignup(n);
			if(n <= 128)
			{
				if(n > 32)
				{
					if(n > 64){
						//(64,128]
						return m_p_128->new_mem(iflag);
					}
					else{
						//(32,64]
						return m_p__64->new_mem(iflag);
					}
				}
				else{
					if(n > 16){
						//(16,32]
						return m_p__32->new_mem(iflag);
					}
					else{
						//(0,16]
						return m_p__16->new_mem(iflag);
					}
				}
			}
			else if(n <= 2048)
			{
				if(n > 512)
				{
					if(n > 1024){
						//(1024,2048]
						return m_p2048->new_mem(iflag);
					}
					else{
						//(512,1024]
						return m_p1024->new_mem(iflag);
					}
				}
				else{
					if(n > 256){
						//(256,512]
						return m_p_512->new_mem(iflag);
					}
					else{
						//(128,256]
						return m_p_256->new_mem(iflag);
					}
				}
			}
			else if(n <= 4096){
				//(2048,4096]
				return m_p4096->new_mem(iflag);
			}
			else if(n <= 8192){
				//(4096,8192]
				return m_p8192->new_mem(iflag);
			}
			char* p = new(std::nothrow)char[n + sizeof(mem_node*)];
			if(p){
				memset(p, 0, sizeof(mem_node*));
				p += sizeof(mem_node*);
			}
			return p;
		}
		void  mem_pool_del(void* p)
		{
			if(!p) return ;
			mem_node* pnode = 0;
			char* pdel = (char*)p - sizeof(mem_node*);
			memcpy(&pnode, pdel, sizeof(mem_node*));
			pnode ? pnode->del_mem(p) : delete pdel, pdel = 0;
		}
	private:
		mem_node* new_node(unsigned int ielemsize, unsigned int ielemcount)
		{
			unsigned int size = ielemsize * ielemcount;
			char* p = new(std::nothrow)char[size];
			if(!p || (size < sizeof(mem_node) + sizeof(mem_node*) + ielemsize)){
				if(p){
					delete p, p = 0;
				}
				return 0;
			}
			char* pend = p + size;
			memset(p, 0, size);
			mem_node* pnode = (mem_node*)p;
			pnode->m_ielemsize = ielemsize;
			p += sizeof(mem_node);
			pnode->m_pmemt = p + sizeof(mem_node*);
			while(1){
				memcpy(p, &pnode, sizeof(mem_node*));//node add
				p += sizeof(mem_node*);
				//elemnext
				char* pnext = p + ielemsize + sizeof(mem_node*);
				if(pnext + ielemsize > pend){
					break;
				}
				memcpy(p, &pnext, sizeof(void*));
				p += ielemsize;//nextnode
			}
			memcpy(&pnode->m_pmemhead, pnode->m_pmemt, sizeof(void*));
			return pnode;
		}
	private:
		mem_node* m_pmemorydelhead;
		mem_node* m_p__16; mem_node* m_p__32;
		mem_node* m_p__64; mem_node* m_p_128;
		mem_node* m_p_256; mem_node* m_p_512;
		mem_node* m_p1024; mem_node* m_p2048;
		mem_node* m_p4096; mem_node* m_p8192;
	};
	template<typename obj>class obj_pool
	{
		template<typename elem> struct node
		{
			node():m_nlock(0), m_pelemhead(0), m_pnodenext(0){}
			volatile long		m_nlock;
			elem*				m_pelemhead;
			node<elem>*	m_pnodenext;
		};
	public:
		int m_pagesize;
		volatile long m_newnodelock;
		node<obj>* m_pnodehead;
		node<obj>* m_pnodetail;
		node<obj>* m_pnodeaddi;
		volatile long m_naddilock;
		obj* m_pobj;
		volatile long m_nobjlock;
	public:
		obj_pool(int inodecount = 10, int ipagesize = 1024 * 8)
			:m_pagesize(ipagesize), m_newnodelock(0), m_pnodehead(0), m_pnodetail(0), m_pnodeaddi(0), m_naddilock(0), m_pobj(0), m_nobjlock(0)
		{
			for(int i = 0; i < inodecount; ++i)
			{
				unsigned int iobjcount = 0;
				node<obj>* pnode = new_node(iobjcount, m_pagesize);
				if(!pnode){continue;}
				if(!m_pnodehead){
					m_pnodetail = pnode, m_pnodehead = pnode;
				}
				else{
					pnode->m_pnodenext = m_pnodehead, m_pnodehead = pnode;
				}
			}
			m_pnodetail->m_pnodenext = m_pnodehead;
			m_pobj = newobj();
		}
		virtual ~obj_pool(void)
		{
			m_pnodetail->m_pnodenext = 0;
			while(m_pnodehead)
			{
				char* pdel = (char*)m_pnodehead;
				m_pnodehead = m_pnodehead->m_pnodenext;
				if(pdel){delete pdel, pdel = 0;}
			}
			m_pnodehead = m_pnodetail = 0;

			if(m_pnodeaddi)
			{
				while(cb_lockcompareexchange(m_naddilock, 1, 0) == 1);
				node<obj>* pdel = m_pnodeaddi;
				while(pdel){
					m_pnodeaddi = m_pnodeaddi->m_pnodenext;
					delete pdel, pdel = m_pnodeaddi;
				}
				cb_lockexchange(m_naddilock, 0);
			}
		}
	public:
		obj* newobj(void)
		{
			node<obj>* phead = m_pnodehead;
			while(1)
			{
				if(m_pobj)//temp
				{
					if(cb_lockcompareexchange(m_nobjlock, 1, 0) == 0)
					{
						if(m_pobj)
						{
							obj* p = m_pobj;
							m_pobj = 0;
							cb_lockexchange(m_nobjlock, 0);
							return p;
						}
						cb_lockexchange(m_nobjlock, 0);
					}
				}
				if(phead->m_pelemhead)
				{
					if(cb_lockcompareexchange(phead->m_nlock, 1, 0) == 0)
					{
						if(phead->m_pelemhead)
						{
							obj* pelem = phead->m_pelemhead;
							memcpy(&phead->m_pelemhead, pelem, sizeof(obj*));
							cb_lockexchange(phead->m_nlock, 0);
							return pelem;
						}
						else{
							cb_lockexchange(phead->m_nlock, 0);
						}
					}
					else{
						phead = phead->m_pnodenext; continue;
					}
				}
				if(cb_lockcompareexchange(phead->m_nlock, 1, 0) == 0)
				{
#pragma region
					if(m_pagesize >= (sizeof(node<obj>) + sizeof(node<obj>*) + sizeof(obj)))
					{
						char* pbegin = new(std::nothrow) char[m_pagesize];
						if(pbegin)
						{
							char* pend = pbegin + m_pagesize;
							memset(pbegin, 0, m_pagesize);
							node<obj>* pnode = (node<obj>*)pbegin;
							pbegin += sizeof(node<obj>);
							obj* pobjhead = (obj*)(pbegin + sizeof(node<obj>*));
							obj* ptail = pobjhead;
							while(1)
							{
								memcpy(pbegin, &phead, sizeof(node<obj>*));
								pbegin += sizeof(node<obj>*);
								char* pnext = pbegin + sizeof(obj) + sizeof(node<obj>*);
								if(pnext + sizeof(obj) <= pend){
									ptail = (obj*)pnext;
								}
								else{
									break;
								}
								memcpy(pbegin, &pnext, sizeof(obj*));
								pbegin += sizeof(obj);
							}
							if(phead->m_pelemhead){
								memcpy(ptail, &phead->m_pelemhead, sizeof(obj*));
							}
							phead->m_pelemhead = pobjhead;
							memcpy(&phead->m_pelemhead, pobjhead, sizeof(obj*));

							while(cb_lockcompareexchange(m_naddilock, 1, 0) == 1);
							if(m_pnodeaddi){
								pnode->m_pnodenext = m_pnodeaddi;
							}
							m_pnodeaddi = pnode;
							cb_lockexchange(m_naddilock, 0);

							cb_lockexchange(phead->m_nlock, 0);
							return pobjhead;
						}
					}
					cb_lockexchange(phead->m_nlock, 0);
					int ilen = sizeof(obj) + sizeof(node<obj>*);
					char* p = new(std::nothrow) char[ilen];
					if(!p){
						return 0;
					}
					memset(p, 0, ilen);
					return (obj*)(p + sizeof(node<obj>*));
#pragma endregion
				}
				else{
					phead = phead->m_pnodenext; continue;
				}
			}
		}
		void delobj(void* p)
		{
			if(!p) return ;
			node<obj>* pnode = 0;
			memcpy(&pnode, (char*)p - sizeof(node<obj>*), sizeof(node<obj>*));
			if(!pnode){
				delete ((char*)p - sizeof(node<obj>*)), p = 0;
				return ;
			}
			if(!m_pobj)
			{
				if(cb_lockcompareexchange(m_nobjlock, 1, 0) == 0)
				{
					if(!m_pobj){
						m_pobj = (obj*)p;
						cb_lockexchange(m_nobjlock, 0);
						return ;
					}
					cb_lockexchange(m_nobjlock, 0);
				}
			}
			while(cb_lockcompareexchange(pnode->m_nlock, 1, 0) == 1);
			memcpy(p, &pnode->m_pelemhead, sizeof(obj*));
			pnode->m_pelemhead = (obj*)p;
			cb_lockexchange(pnode->m_nlock, 0);
		}
	private:
		node<obj>* new_node(unsigned int& iobjcount, int pagesize = 1024 * 8)
		{
			char* pbegin = new(std::nothrow) char[pagesize];
			iobjcount = (pagesize - sizeof(node<obj>))/(sizeof(node<obj>*) + sizeof(obj));
			if(!pbegin || iobjcount <= 0){
				if(pbegin){delete []pbegin, pbegin = 0;}
				return 0;
			}
			char* pend = pbegin + pagesize;
			memset(pbegin, 0, pagesize);
			node<obj>* pnode = (node<obj>*)pbegin;
			pbegin += sizeof(node<obj>);
			pnode->m_pelemhead	= (obj*)(pbegin + sizeof(node<obj>*));
			while(1){
				memcpy(pbegin, &pnode, sizeof(node<obj>*));//nodehead
				pbegin += sizeof(node<obj>*);//skip elem
				//elemnext
				char* pnext = pbegin + sizeof(obj) + sizeof(node<obj>*);
				if(pnext + sizeof(obj) > pend){
					break;
				}
				memcpy(pbegin, &pnext, sizeof(obj*));
				pbegin += sizeof(obj);//nextnode
			}
			return pnode;
		}
	};
};

namespace cb_space_timer
{
	typedef struct callbackparams
	{
#ifdef WIN32
		callbackparams()
			:pcallback(0),pparams(0),itimerid(0){
#else
		callbackparams()
			:pcallback(0),pparams(0),idelay(0),istop(0){
#endif
		}
		pvoid_proc_pvoid pcallback;
		void* pparams;
#ifdef WIN32
		int itimerid;
#else
		int idelay;
		volatile int istop;
#endif
	}callbackparams, *pcallbackparams;
#ifndef WIN32
	void* __cb_timer_proc__(void* pparams)
	{
		pcallbackparams pcbparams = (pcallbackparams)pparams;
		if(pcbparams->pcallback)
		{
			timeval tv;
			int sec = pcbparams->idelay / 1000; int usec = (pcbparams->idelay % 1000) * 1000;
			while(cb_lockcompareexchange(pcbparams->istop, 0, 0) == 0)
			{
				tv.tv_sec = sec; tv.tv_usec = usec;
				if((select(0, 0, 0, 0, &tv) == 0))
					pcbparams->pcallback(pcbparams->pparams);
			}
			cb_lockexchange(pcbparams->istop, 2);
		}
		return 0;
	}
#endif
	class cb_timer
	{
	public:
		cb_timer():m_pcallbackparams(0){}
		virtual ~cb_timer(){stop();}
	public:
		int start(int idelay, pvoid_proc_pvoid callback, void* pcallbackparam)
		{
			m_pcallbackparams = new(std::nothrow) callbackparams;
			if(!m_pcallbackparams){
				return -1;
			}
			m_pcallbackparams->pcallback = callback;
			m_pcallbackparams->pparams = pcallbackparam;
#ifdef WIN32
			m_pcallbackparams->itimerid = timeSetEvent(idelay, 1, __cb_timer_proc__, (DWORD)m_pcallbackparams, 1);
			if(m_pcallbackparams->itimerid == 0){
				delete m_pcallbackparams, m_pcallbackparams = 0;
				return -2;
			}
#else
			m_pcallbackparams->idelay = idelay;
			cb_thread_fd tfd;
			cb_create_thread(tfd, __cb_timer_proc__, m_pcallbackparams);
			if(cb_thread_fail(tfd)){
				return -3;
			}
#endif
			return 0;
		}
		void stop(void)
		{
			if(m_pcallbackparams)
			{
#ifdef WIN32
				if(m_pcallbackparams->itimerid != 0) timeKillEvent(m_pcallbackparams->itimerid);
#else
				cb_lockexchange(m_pcallbackparams->istop, 1);
				while(cb_lockcompareexchange(m_pcallbackparams->istop, 2, 2) != 2){
					em_sleep(200);
				}
#endif
				delete m_pcallbackparams, m_pcallbackparams = 0;
			}
		}
	private:
#ifdef WIN32
		static void __stdcall __cb_timer_proc__(unsigned int , unsigned int , unsigned long pparams, unsigned long , unsigned long)
		{
			pcallbackparams pcbparams = (pcallbackparams)pparams;
			if(pcbparams->pcallback){
				pcbparams->pcallback(pcbparams->pparams);
			}
			return ;
		}
#endif
	private:
		pcallbackparams m_pcallbackparams;
	};
};

namespace cb_space_poling{
/*
void* proc(void* p)
{
test* pt = (test*)p;
if(!pt){
return (void*)-1;
}
do{
printf("-----%d\n", pt->i);
test* pdel = pt;
pt = pt->pnext;
delete pdel, pdel = 0;
}while(pt);

return 0;
}

cb_space_poling::ctimerpoling<test> t;
t.start(1000, 5, proc);
int i = 0;
while(i++ < 200){
test* pt = new test;
pt->i = i;
Sleep(10);
t.add(pt, &pt->pnext);
}
getchar();
t.stop();
*/
template<typename t> class ctimerpoling
{
	struct pollnode
	{
		volatile unsigned long m_nlock;
		t* m_pthead;
		pollnode* m_pprev;
		pollnode* m_pnext;
	};
	struct cb_params{
		pvoid_proc_pvoid proc;
		ctimerpoling* pthis;
	};
	ctimerpoling(const ctimerpoling& ref);
	ctimerpoling& operator=(const ctimerpoling& ref);
public:
	ctimerpoling():m_phead(0),m_pdel(0){}
	virtual ~ctimerpoling()
	{
		stop();
	}
public:
	int start(int idelaytime, int ipolingcount, pvoid_proc_pvoid proc)
	{
		if(idelaytime <= 0 || ipolingcount <= 0 || !proc){
			return -1;
		}
		if(!createloop(ipolingcount)){
			return -2;
		}
		m_params.proc	 = proc;
		m_params.pthis   = this;
		m_timer.start(idelaytime/ipolingcount, invoke, &m_params);
		return 0;
	}
	int add(t* pthead, t** pptailnext)
	{
		if(!pthead || !pptailnext){
			return -1;
		}
		pollnode* paddnode = m_phead->m_pprev;
		while(cb_lockcompareexchange(paddnode->m_nlock, 1, 0) == 1);
		if(paddnode->m_pthead){
			*pptailnext = paddnode->m_pthead;
		}
		paddnode->m_pthead = pthead;
		cb_lockexchange(paddnode->m_nlock, 0);
		return 0;
	}
	void stop(void)
	{
		m_timer.stop();
		m_phead = 0;
		if(m_pdel){
			delete []m_pdel, m_pdel = 0;
		}
	}
private:
	int invoke(pvoid_proc_pvoid proc)
	{
		if(!proc){
			return -1;
		}
		while(cb_lockcompareexchange(m_phead->m_nlock, 1, 0) == 1);
		pollnode* pdelnode = m_phead;
		if(pdelnode->m_pthead){
			proc(pdelnode->m_pthead);
			pdelnode->m_pthead = 0;
		}
		m_phead = pdelnode->m_pnext;
		cb_lockexchange(pdelnode->m_nlock, 0);
		return 0;
	}
private:
	static void* invoke(void* pparams)
	{
		cb_params* p = (cb_params*)pparams;
		if(!p){
			return (void*)-1;
		}
		p->pthis->invoke(p->proc);
		return 0;
	}
	pollnode* createloop(int ipolingcount)
	{
		if(ipolingcount <= 0)
			return 0;
		ipolingcount += 1;//
		m_pdel = new(std::nothrow) pollnode[ipolingcount];
		if(!m_pdel)
			return 0;
		memset(m_pdel, 0, sizeof(pollnode) * ipolingcount);
		pollnode* ptail = 0;
		for(int i = 0; i < ipolingcount - 1; ++i)
		{
			m_phead = &m_pdel[i]; ptail = &m_pdel[i + 1];
			m_phead->m_pnext = ptail;
			ptail->m_pprev = m_phead;
		}
		ptail->m_pnext = m_pdel;
		m_pdel->m_pprev = ptail;
		m_phead = m_pdel;
		return m_phead;
	}
private:
	pollnode* m_phead;
	pollnode* m_pdel;
	cb_params m_params;
	cb_space_timer::cb_timer m_timer;
};

template<typename t> class ctimerpolingex;
template<typename t> struct pollnodeex;
template<typename t> struct telem
{
	telem():pprev(0),pnext(0),ppollnodeex(0){}
	t data;
	telem* pprev;
	telem* pnext;
	pollnodeex<t>* ppollnodeex;
};
template<typename t> struct pollnodeex
{
	pollnodeex():m_nlock(0),m_pthead(0),m_pprev(0),m_pnext(0){}
	volatile unsigned long m_nlock;
	telem<t>* m_pthead;
	pollnodeex* m_pprev;
	pollnodeex* m_pnext;
};
template<typename t> struct cb_paramsex
{
	cb_paramsex():proc(0),pthis(0){}
	pvoid_proc_pvoid proc;
	ctimerpolingex<t>* pthis;
};
#ifndef WIN32
template<typename t> void* __cb_poling_proc__(void* pparams);
#endif
/*
void* proc2(void* p)
{
test* pt = (test*)p;
if(!pt){
printf("err\n");
return (void*)-1;
}
printf("----%d\n", pt->i);
return 0;
}

cb_space_poling::ctimerpolingex<test> t;
t.start(1000, 5, proc2);
int i = 0;
void* p = 0;
while(i++ < 200){
test pt;
pt.i = i;
Sleep(10);
p = t.add(pt);
if(i%7 == 0){
printf("del %d\n", i);
t.del(p);
}
}
getchar();
t.stop();
*/
template<typename t> class ctimerpolingex
{
#ifndef WIN32
	friend void* __cb_poling_proc__<t>(void* pparams);
#endif
public:
	ctimerpolingex()
		:m_handlehead(0),m_handlelock(0),m_nhandleprocexitflag(0),m_nhandleproccount(0),m_phead(0),m_pdel(0){}
	virtual ~ctimerpolingex(){
		stop();
	}
public:
	int start(int idelaytime, int ipolingcount, pvoid_proc_pvoid proc, int iproccount = 2);
	int stop(void);
	void* add(const t& _data);
	int del(void* p);
private:
	int handletimeout(pvoid_proc_pvoid proc);
	int invokeex(void);
	telem<t>* get(void);
	void add(telem<t>* phead, telem<t>* ptail);
	void releasehandletelem(void);
	static void* invokeex(void* pparams);
#ifdef WIN32
	static unsigned int __stdcall __cb_poling_proc__(void* pparams);
#endif
	pollnodeex<t>* createloop(int ipolingcount);
public:
	volatile long m_nhandleprocexitflag;
	volatile long m_nhandleproccount;
private:
	telem<t>* m_handlehead;
	volatile long m_handlelock;
	pollnodeex<t>* m_phead;
	pollnodeex<t>* m_pdel;
	cb_paramsex<t> m_paramsex;
	cb_space_memorypool::obj_pool<telem<t> > m_tpool;
	cb_space_timer::cb_timer m_timer;
};
template<typename t>
#ifndef WIN32
void* __cb_poling_proc__(void* pparams)
#else
unsigned int __stdcall ctimerpolingex<t>::__cb_poling_proc__(void* pparams)
#endif
{
	cb_paramsex<t>* p = (cb_paramsex<t>*)pparams;
	if(!p)
#ifdef WIN32
		return -1;
#else
		return (void*)-1;
#endif
	int iret = 0;
	while(1)
	{
		if(cb_lockcompareexchange(p->pthis->m_nhandleprocexitflag, 1, 1) == 1)
		{
			cb_lockexsub(p->pthis->m_nhandleproccount, 1);
			break;
		}
		iret = p->pthis->handletimeout(p->proc);
		if(iret == -1){//err
			cb_lockexsub(p->pthis->m_nhandleproccount, 1);
			break;
		}
		if(iret != 0)
		{
			cb_sleep(100);
		}
	}
	return 0;
}
template<typename t>
int ctimerpolingex<t>::start(int idelaytime, int ipolingcount, pvoid_proc_pvoid proc, int iproccount)
{
	if(idelaytime <= 0 || ipolingcount <= 0 || !proc){
		return -1;
	}
	if(!createloop(ipolingcount)){
		return -2;
	}
	m_timer.start(idelaytime/ipolingcount, invokeex, this);
	m_paramsex.proc = proc;
	m_paramsex.pthis = this;
	int i = 0;
	while(i++ < iproccount)
	{
		cb_thread_fd tfd;
#ifdef WIN32
		cb_create_thread(tfd, __cb_poling_proc__, &m_paramsex);
#else
		cb_create_thread(tfd, __cb_poling_proc__<t>, &m_paramsex);
#endif
		if(cb_thread_fail(tfd)){
			return -3;
		}
		cb_lockexadd(m_nhandleproccount, 1);
	}
	return 0;
}
template<typename t>
int ctimerpolingex<t>::stop(void)
{
	//release pool obj
	releasehandletelem();
	//stop handle timeout proc (proc count)
	cb_lockexchange(m_nhandleprocexitflag, 1);
	while(1)
	{
		if(cb_lockcompareexchange(m_nhandleproccount, 0, 0) == 0)
		{
			break;
		}
		cb_sleep(100);
	}
	printf("stop...\n");
	//timer
	m_timer.stop();
	//delete memory
	if(m_pdel){
		delete []m_pdel, m_pdel = 0;
	}
	return 0;
}
template<typename t>
void* ctimerpolingex<t>::add(const t& _data)
{
	telem<t>* ptdata = m_tpool.newobj();
	if(!ptdata)
		return (void*)-1;
	pollnodeex<t>* padd = m_phead->m_pprev;
	while(cb_lockcompareexchange(padd->m_nlock, 1, 0) == 1) padd = m_phead->m_pprev;
	memcpy(&ptdata->data, &_data, sizeof(_data));
	ptdata->ppollnodeex = padd;
	if(padd->m_pthead){
		padd->m_pthead->pprev = ptdata;
		ptdata->pnext = padd->m_pthead;
	}
	padd->m_pthead = ptdata;
	cb_lockexchange(padd->m_nlock, 0);
	return ptdata;
}
template<typename t>
int ctimerpolingex<t>::del(void* p)
{
	telem<t>* ptdata = (telem<t>*)p;
	if(!ptdata)
		return -1;
	pollnodeex<t>* pnode = ptdata->ppollnodeex;
	if(pnode)
	{
		while(cb_lockcompareexchange(pnode->m_nlock, 1, 0) == 1);
		if(ptdata->ppollnodeex)
		{
			if(pnode->m_pthead == ptdata){
				pnode->m_pthead = ptdata->pnext;
				if(ptdata->pnext){
					ptdata->pnext->pprev = 0;
				}
			}
			else{
				ptdata->pprev->pnext = ptdata->pnext;
				if(ptdata->pnext){
					ptdata->pnext->pprev = ptdata->pprev;
				}
			}
			ptdata->ppollnodeex = 0;
		}
		ptdata->pnext = ptdata->pprev = 0;
		cb_lockexchange(pnode->m_nlock, 0);
	}
	else{
		ptdata->pnext = ptdata->pprev = 0;
	}
	m_tpool.delobj(ptdata);
	return 0;
}
template<typename t>
int ctimerpolingex<t>::handletimeout(pvoid_proc_pvoid proc)
{
	if(!proc){
		return -1;
	}
	telem<t>* ptelem = get();
	if(!ptelem){
		return 1;
	}
	proc(&ptelem->data);
	m_tpool.delobj(ptelem);
	return 0;
}
template<typename t>
int ctimerpolingex<t>::invokeex(void)
{
	while(cb_lockcompareexchange(m_phead->m_nlock, 1, 0) == 1);
	pollnodeex<t>* pdelnode = m_phead;
	m_phead = m_phead->m_pnext;
	if(pdelnode->m_pthead){
		telem<t>* ptelemhead = pdelnode->m_pthead;
		telem<t>* ptelemtail = 0;
		while(ptelemhead){
			ptelemtail = ptelemhead;
			ptelemhead->ppollnodeex = 0;//*
			ptelemhead = ptelemhead->pnext;
		}
		add(pdelnode->m_pthead, ptelemtail);
		pdelnode->m_pthead = 0;
	}
	cb_lockexchange(pdelnode->m_nlock, 0);
	return 0;
}
template<typename t>
telem<t>* ctimerpolingex<t>::get(void)
{
	telem<t>* pret = 0;
	while(cb_lockcompareexchange(m_handlelock, 1, 0) == 1);
	if(m_handlehead)
	{
		pret = m_handlehead;
		m_handlehead = m_handlehead->pnext;
		if(m_handlehead)
			m_handlehead->pprev = 0;
		pret->pnext = 0;
	}
	cb_lockexchange(m_handlelock, 0);
	return pret;
}
template<typename t>
void ctimerpolingex<t>::add(telem<t>* phead, telem<t>* ptail)
{
	if(!phead)
		return ;
	while(cb_lockcompareexchange(m_handlelock, 1, 0) == 1);
	if(ptail){
		if(m_handlehead){
			ptail->pnext = m_handlehead;
			m_handlehead->pprev = ptail;
		}
	}
	else{
		if(m_handlehead){
			phead->pnext = m_handlehead;
			m_handlehead->pprev = phead;
		}
	}
	m_handlehead = phead;
	cb_lockexchange(m_handlelock, 0);
}
template<typename t>
void ctimerpolingex<t>::releasehandletelem(void)
{
	if(m_handlehead)
	{
		while(cb_lockcompareexchange(m_handlelock, 1, 0) == 1);
		telem<t>* pdel = m_handlehead;
		while(pdel)
		{
			m_handlehead = m_handlehead->pnext;
			if(m_handlehead){
				m_handlehead->pprev = 0;
			}
			pdel->pnext = 0;
			m_tpool.delobj(pdel);
			pdel = m_handlehead;
		}
		m_handlehead = 0;
		cb_lockexchange(m_handlelock, 0);
	}
}
template<typename t>
void* ctimerpolingex<t>::invokeex(void* pparams)
{
	ctimerpolingex<t>* pthis = (ctimerpolingex<t>*)pparams;
	if(!pthis){
		return (void*)-1;
	}
	pthis->invokeex();
	return 0;
}
template<typename t>
pollnodeex<t>* ctimerpolingex<t>::createloop(int ipolingcount)
{
	if(ipolingcount <= 0)
		return 0;
	ipolingcount += 1;//
	m_pdel = new(std::nothrow) pollnodeex<t>[ipolingcount];
	if(!m_pdel)
		return 0;
	memset(m_pdel, 0, sizeof(pollnodeex<t>) * ipolingcount);
	pollnodeex<t>* ptail = 0;
	for(int i = 0; i < ipolingcount - 1; ++i)
	{
		m_phead = &m_pdel[i]; ptail = &m_pdel[i + 1];
		m_phead->m_pnext = ptail;
		ptail->m_pprev = m_phead;
	}
	ptail->m_pnext = m_pdel;
	m_pdel->m_pprev = ptail;
	m_phead = m_pdel;
	return m_phead;
}
};


