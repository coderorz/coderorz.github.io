#pragma once
//#include "MemoryPool.h"
#define ReleaseSocket(SOCK) if(SOCK != INVALID_SOCKET){closesocket(SOCK),SOCK = INVALID_SOCKET;}
#include "map"
#include "vector"
/*/////////////////////////////////////////////////////////////
	IOCPServer Context
/////////////////////////////////////////////////////////////*/
typedef enum IO_TYPE
{
	IO_TYPE_NULL,
	IO_TYPE_CONN,
	IO_TYPE_DOFR,
	IO_TYPE_RECV,
}IO_TYPE;

#define LISTEN_IOCONTEXT_BUFFSIZE 64
typedef struct LISTEN_IO_CONTEXT
{
	void reset(SOCKET sock)
	{
		m_listenSock = sock;
		m_listenWsaBuff.buf = m_listenBuff;
		m_listenWsaBuff.len = sizeof(m_listenBuff);
		memset(&m_listenIOOL, 0, sizeof(m_listenIOOL));
		memset(m_listenBuff, 0, sizeof(m_listenBuff));
	}
	OVERLAPPED		m_listenIOOL;
	SOCKET			m_listenSock;
	WSABUF			m_listenWsaBuff;
	CHAR			m_listenBuff[LISTEN_IOCONTEXT_BUFFSIZE];
}LISTEN_IO_CONTEXT, *PLISTEN_IO_CONTEXT;

typedef struct IO_CONTEXT
{
	~IO_CONTEXT()
	{
		m_ioWsaBuff.buf = buf;
		m_ioWsaBuff.len = 1;
		m_ioType = IO_TYPE_NULL;
	}
	
	void reset(IO_TYPE iotype)
	{
		m_ioType = iotype;
		memset(&m_ioOverLappend, 0, sizeof(m_ioOverLappend));
	}

	OVERLAPPED		m_ioOverLappend;
	IO_TYPE			m_ioType;
	WSABUF			m_ioWsaBuff;
	char			buf[1024];
}IO_CONTEXT, *PIO_CONTEXT;

struct loopnode;
typedef struct SOCKET_CONTEXT{
	void reset(SOCKET sock){
		m_sock = sock;
		memset(&m_clientAddr, 0, sizeof(SOCKADDR_IN));
		m_pnext = m_pprev = 0;
		
		m_ploopnode = 0;
		m_nIoCount	= 0;
		m_lock		= 0;
	}
	int add(){
		int nRet(0);
		while(InterlockedCompareExchange(&m_lock, 1, 0) == 1);
		{
			++m_nIoCount;
			nRet = m_nIoCount;
		}
		InterlockedExchange(&m_lock, 0);
		return nRet;
	}
	int del(){
		int nRet(-1);
		while(InterlockedCompareExchange(&m_lock, 1, 0) == 1);
		{
			if(m_nIoCount == 0){
				nRet = -2;
			}
			else{
				--m_nIoCount;
				nRet = m_nIoCount;
			}
		}
		InterlockedExchange(&m_lock, 0);
		return nRet;
	}
	volatile unsigned long m_nIoCount;
	SOCKET			m_sock;
	SOCKADDR_IN		m_clientAddr;
	struct SOCKET_CONTEXT* m_pnext;
	struct SOCKET_CONTEXT* m_pprev;
	struct loopnode* m_ploopnode;
	volatile unsigned long m_lock;
}SOCKET_CONTEXT, *PSOCKET_CONTEXT;

class boob_log{
public:
	boob_log():lock(0)
	{
	}

	~boob_log(){
		if(strlist.size()){
			FILE * fp(NULL);
			int flag = fopen_s(&fp, "D:\\MyLog\\log.txt","a");
			for(std::vector<std::string>::iterator iter = strlist.begin(); iter != strlist.end(); ++iter){
				if(fp)
					fwrite(iter->c_str(), iter->size(), 1, fp);
			}
			if(fp){
				fflush(fp);
				fclose(fp);
			}
		}
		strlist.clear();
	}
	void writelog(std::string str){
		//return ;
		while(InterlockedCompareExchange(&lock, 1, 0) == 1);
		{
			if(strlist.size() >= 10){
				FILE * fp(NULL);
				strlist.push_back(str);
				int flag = fopen_s(&fp, "D:\\MyLog\\log.txt","a");
				for(std::vector<std::string>::iterator iter = strlist.begin(); iter != strlist.end(); ++iter){
					if(fp)
						fwrite(iter->c_str(), iter->size(), 1, fp);
				}
				if(fp){
					fflush(fp);
					fclose(fp);
				}
				strlist.clear();
			}
			else{
				strlist.push_back(str);
			}
		}
		InterlockedExchange(&lock, 0);
	}
private:
	volatile long lock;
	std::vector<std::string> strlist;
};

#define  NODE_SIZE
typedef  PSOCKET_CONTEXT loopelem;

struct loopnode{
	volatile unsigned long m_lock;
	struct loopnode* m_pnext;
	struct loopnode* m_pprev;
	loopelem	m_pelemhead;
};

/*//////////////////////////////////////////
 ______________________________
|			 ________          |
|___________|_____	 |  _______|__   _________
|_______	|	 _|__|_|_____ _|__|_|_____
   |head|---^   | f | n 	 | f | n 	  |
   |next|--		| r | e  DATA| r | e  DATA|
  ________|		| e | x 	 | e | x 	  |
 |				| e | t 	 | e | t 	  |
  ->____
   |head|	...
   |next|	...
//////////////////////////////////////////*/

#ifdef WIN32
#define cb_lockexchange(k, v) InterlockedExchange(&k, v)
#define cb_lockcompareexchange(k, v, c) InterlockedCompareExchange(&k, v, c)
#define cb_lockexadd(k, v) InterlockedExchangeAdd(&k,  v)
#define cb_lockexsub(k, v) InterlockedExchangeAdd(&k, -v)
#define cb_sleep(t) Sleep(t);
#else
#include<pthread>
#define cb_lockexchange(k, v) __sync_val_compare_and_swap(&k, k, v)
#define cb_lockcompareexchange(k, v, c) __sync_val_compare_and_swap(&k, c, v)
#define cb_lockexadd(k, v) __sync_fetch_and_add(&k, v);
#define cb_lockexsub(k, v) __sync_fetch_and_sub(&k, v);
#define cb_sleep(t) usleep(t * 1000);
#endif


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
		struct mem_node* m_pnextnode;	//指向下个node
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
		void* new_elem(struct mem_node* pnode, unsigned int ielemcount = 1024, bool bnewelem = false, void** ptail = 0)
		{
			//bnewelem = false and ptail = 0, or ptail can not be 0
			volatile static long lock = 0;
			unsigned int ielemsize = pnode->m_ielemsize;
			while(cb_lockcompareexchange(lock, 1, 0) == 1);
			if(!bnewelem){
				char* p = new(std::nothrow)char[ielemsize + sizeof(void*)];
				if(p)
				{
					memset(p, 0, sizeof(struct mem_node*));
					p += sizeof(struct mem_node*);//0 flag for system delete
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
			void* phead = p + sizeof(struct mem_node*);
			while(1)
			{
				//node add
				memcpy(p, &pnode, sizeof(struct mem_node*));
				p += sizeof(struct mem_node*);
				*ptail = p;//set last elem for insert list
				//elemnext
				char* pnext = p + ielemsize + sizeof(struct mem_node*);
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
		//m_bmap[16]	= 4096;	m_bmap[32]	= 4096;	m_bmap[64]	 = 4096; m_bmap[128]  = 2048; m_bmap[256] = 2048;
		//m_bmap[512]	= 1024;	m_bmap[1024]= 1024;	m_bmap[2048] = 1024; m_bmap[4096] = 1024; m_bmap[8192]= 1024;
		m_bmap[16]	= 100;	m_bmap[32]	= 100;	m_bmap[64]	 = 100; m_bmap[128]  = 100; m_bmap[256] = 100;
		m_bmap[512]	= 100;	m_bmap[1024]= 100;	m_bmap[2048] = 100; m_bmap[4096] = 100; m_bmap[8192]= 100;
		unsigned int arr[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};
		struct mem_node* pnodehead = 0;
		for(int i = sizeof(arr)/sizeof(arr[0]) - 1; i >= 0; --i)
		{
			struct mem_node* pnode = new_node(arr[i], m_bmap[arr[i]]);
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
		struct mem_node* phead = m_pmemorydelhead;
		while(phead)
		{
			void* p = phead->m_pblock;
			while(p){
				void* pnext = 0;
				memcpy(&pnext, p, sizeof(void*));
				delete p, p = pnext;
			}
			void* pdel = phead;
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
		char* p = new(std::nothrow)char[n + sizeof(struct mem_node*)];
		if(p){
			memset(p, 0, sizeof(struct mem_node*));
			p += sizeof(struct mem_node*);
		}
		return p;
	}
	void  mem_pool_del(void* p)
	{
		if(!p) return ;
		struct mem_node* pnode = 0;
		char* pdel = (char*)p - sizeof(struct mem_node*);
		memcpy(&pnode, pdel, sizeof(struct mem_node*));
		pnode ? pnode->del_mem(p) : delete pdel, pdel = 0;
	}
private:
	struct mem_node* new_node(unsigned int ielemsize, unsigned int ielemcount)
	{
		unsigned int size = ielemsize * ielemcount;
		char* p = new(std::nothrow)char[size];
		if(!p || (size < sizeof(struct mem_node) + sizeof(struct mem_node*) + ielemsize)){
			if(p){
				delete p, p = 0;
			}
			return 0;
		}
		char* pend = p + size;
		memset(p, 0, size);
		struct mem_node* pnode = (struct mem_node*)p;
		pnode->m_ielemsize = ielemsize;
		p += sizeof(struct mem_node);
		pnode->m_pmemt = p + sizeof(struct mem_node*);
		while(1){
			memcpy(p, &pnode, sizeof(struct mem_node*));//node add
			p += sizeof(struct mem_node*);
			//elemnext
			char* pnext = p + ielemsize + sizeof(struct mem_node*);
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
	struct mem_node* m_pmemorydelhead;
	struct mem_node* m_p__16; struct mem_node* m_p__32;
	struct mem_node* m_p__64; struct mem_node* m_p_128;
	struct mem_node* m_p_256; struct mem_node* m_p_512;
	struct mem_node* m_p1024; struct mem_node* m_p2048;
	struct mem_node* m_p4096; struct mem_node* m_p8192;
};

template<typename obj> class obj_pool
{
	template<typename elem> struct node
	{
		node():m_nlock(0), m_pelemhead(0), m_pnodenext(0){}
		volatile long		m_nlock;
		elem*				m_pelemhead;
		struct node<elem>*	m_pnodenext;
	};
public:
	int m_pagesize; /*volatile long m_nadd;*/
	volatile long m_nnodecount;
	volatile long m_newnodelock;
	volatile unsigned int m_nsize;
	struct node<obj>* m_pnodehead;
	struct node<obj>* m_pnodetail;
	obj* m_pobj;
	volatile long m_nobjlock;
public:
	obj_pool(int inodecount = 10, int ipagesize = 1024 * 8)
		:m_pagesize(ipagesize), /*m_nadd(0),*/m_nnodecount(inodecount), m_newnodelock(0), m_nsize(0), m_pnodehead(0), m_pnodetail(0), m_pobj(0), m_nobjlock(0)
	{
		for(int i = 0; i < m_nnodecount; ++i)
		{
			unsigned int iobjcount = 0;
			struct node<obj>* pnode = new_node(iobjcount, m_pagesize);
			if(!pnode){continue;}
			if(!m_pnodehead){
				m_pnodetail = pnode, m_pnodehead = pnode;
			}
			else{
				pnode->m_pnodenext = m_pnodehead, m_pnodehead = pnode;
			}
			m_nsize += iobjcount;
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
	}
public:
	obj* newobj(void)
	{
		struct node<obj>* phead = m_pnodehead;
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
			if(cb_lockcompareexchange(phead->m_nlock, 1, 0) == 0)
			{
				cb_lockexsub(m_nnodecount, 1);
				if(phead->m_pelemhead)
				{
					obj* pelem = phead->m_pelemhead;
					memcpy(&phead->m_pelemhead, pelem, sizeof(obj*));
					cb_lockexsub(m_nsize, 1);
					cb_lockexchange(phead->m_nlock, 0);
					cb_lockexadd(m_nnodecount, 1);
					return pelem;
				}
				cb_lockexchange(phead->m_nlock, 0);
				cb_lockexadd(m_nnodecount, 1);
			}
			unsigned long nnodecount = cb_lockexadd(m_nnodecount, 0);
			if(cb_lockexadd(m_nsize, 0) < nnodecount || nnodecount < 1)
			{
				if(cb_lockcompareexchange(m_newnodelock, 1, 0) == 0)
				{
					if(cb_lockexadd(m_nsize, 0) > (nnodecount << 1))
					{
						cb_lockexchange(m_newnodelock, 0);
						phead = m_pnodetail; continue;
					}
					unsigned int iobjcount = 0;
					struct node<obj>* pnode = new_node(iobjcount, m_pagesize);
					if(pnode)
					{
						obj* pelem = pnode->m_pelemhead;
						memcpy(&pnode->m_pelemhead, pelem, sizeof(obj*));
						while(cb_lockcompareexchange(m_pnodetail->m_nlock, 1, 0) == 1);
						pnode->m_pnodenext = m_pnodetail->m_pnodenext;
						m_pnodetail->m_pnodenext = pnode;
						cb_lockexadd(m_nsize, iobjcount - 1);
						struct node<obj>* ptail = m_pnodetail;
						m_pnodetail = pnode;
						cb_lockexchange(ptail->m_nlock, 0);
						cb_lockexadd(m_nnodecount, 1);
						cb_lockexchange(m_newnodelock, 0);
						return pelem;
					}
					else{
						cb_lockexchange(m_newnodelock, 0);
						return 0;
					}
				}
				else{
					int ilen = sizeof(obj) + sizeof(struct node<obj>*);
					char* p = new(std::nothrow) char[ilen];
					if(!p){
						return 0;
					}
					memset(p, 0, ilen);
					return (obj*)(p + sizeof(struct node<obj>*));
				}
			}
			else{
				phead = phead->m_pnodenext;
			}
		}
	}
	void delobj(void* p)
	{
		if(!p) return ;
		struct node<obj>* pnode = 0;
		memcpy(&pnode, (char*)p - sizeof(struct node<obj>*), sizeof(struct node<obj>*));
		if(!pnode){
			delete ((char*)p - sizeof(struct node<obj>*)), p = 0;
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
		cb_lockexadd(m_nsize, 1);
		cb_lockexchange(pnode->m_nlock, 0);
	}
private:
	struct node<obj>* new_node(unsigned int& iobjcount, int pagesize = 1024 * 8)
	{
		char* pbegin = new(std::nothrow) char[pagesize];
		iobjcount = (pagesize - sizeof(struct node<obj>))/(sizeof(struct node<obj>*) + sizeof(obj));
		if(!pbegin || iobjcount <= 0){
			if(pbegin){delete []pbegin, pbegin = 0;}
			return 0;
		}
		char* pend = pbegin + pagesize;
		memset(pbegin, 0, pagesize);
		struct node<obj>* pnode = (struct node<obj>*)pbegin;
		pbegin += sizeof(struct node<obj>);
		pnode->m_pelemhead	= (obj*)(pbegin + sizeof(struct node<obj>*));
		while(1){
			memcpy(pbegin, &pnode, sizeof(struct node<obj>*));//nodehead
			pbegin += sizeof(struct node<obj>*);//skip elem
			//elemnext
			char* pnext = pbegin + sizeof(obj) + sizeof(struct node<obj>*);
			if(pnext + sizeof(obj) > pend){
				break;
			}
			memcpy(pbegin, &pnext, sizeof(obj*));
			pbegin += sizeof(obj);//nextnode
		}
		return pnode;
	}
};


//原理：调用GetProcessTimes()，并与上次调用得到的结果相减，即得到某段时间内CPU的使用时间  
//C++ 获取特定进程规定CPU使用率  原文：http://blog.csdn.net/liuqx97bb/article/details/52058657  
#include <stdint.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <iostream>
#include <vector>
class CPUusage {   
private:  
	typedef long long          int64_t;  
	typedef unsigned long long uint64_t;  
	HANDLE _hProcess;    
	int _processor;    //cpu数量    
	int64_t _last_time;         //上一次的时间    
	int64_t _last_system_time;    


	// 时间转换    
	uint64_t file_time_2_utc(const FILETIME* ftime){  
		LARGE_INTEGER li;  

		li.LowPart = ftime->dwLowDateTime;  
		li.HighPart = ftime->dwHighDateTime;  
		return li.QuadPart;  
	}  

	// 获得CPU的核数    
	int get_processor_number()
	{  
		SYSTEM_INFO info;  
		GetSystemInfo(&info);  
		return info.dwNumberOfProcessors;  
	}  

	//初始化  
	void init()  
	{  
		_last_system_time = 0;  
		_last_time = 0;  
		_hProcess = 0;  
	}  

	//关闭进程句柄  
	void clear()  
	{  
		if (_hProcess) {  
			CloseHandle(_hProcess);  
			_hProcess = 0;  
		}  
	}  

public:  
	CPUusage(DWORD ProcessID) {   
		init();   
		_processor = get_processor_number();  
		setpid(ProcessID);  
	}  
	CPUusage() { init(); _processor = get_processor_number(); }  
	~CPUusage() { clear(); }  

	DWORD GetProcessIDByName(const TCHAR* pName)
	{
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (INVALID_HANDLE_VALUE == hSnapshot) {
			return NULL;
		}
		PROCESSENTRY32 pe = { sizeof(pe) };
		for (BOOL ret = Process32First(hSnapshot, &pe); ret; ret = Process32Next(hSnapshot, &pe)) {
			if (_tcscmp(pe.szExeFile, pName) == 0) {
				CloseHandle(hSnapshot);
				return pe.th32ProcessID;
			}
			//printf("%-6d %s\n", pe.th32ProcessID, pe.szExeFile);
		}
		CloseHandle(hSnapshot);
		return 0;
	}

	//返回值为进程句柄，可判断OpenProcess是否成功  
	HANDLE setpid(DWORD ProcessID) {   
		clear();    //如果之前监视过另一个进程，就先关闭它的句柄  
		init();   
		return _hProcess= OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, ProcessID);   
	}  

	//-1 即为失败或进程已退出； 如果成功，首次调用会返回-2（中途用setpid更改了PID后首次调用也会返回-2）  
	float get_cpu_usage(){  

		FILETIME now;  
		FILETIME creation_time;  
		FILETIME exit_time;  
		FILETIME kernel_time;  
		FILETIME user_time;  
		int64_t system_time;  
		int64_t time;  
		int64_t system_time_delta;  
		int64_t time_delta;  

		DWORD exitcode;  

		float cpu = -1;  

		if (!_hProcess) return -1;  

		GetSystemTimeAsFileTime(&now);  

		//判断进程是否已经退出  
		GetExitCodeProcess(_hProcess, &exitcode);    
		if (exitcode != STILL_ACTIVE) {  
			clear();  
			return -1;  
		}  

		//计算占用CPU的百分比  
		if (!GetProcessTimes(_hProcess, &creation_time, &exit_time, &kernel_time, &user_time))  
		{  
			clear();  
			return -1;  
		}  
		system_time = (file_time_2_utc(&kernel_time) + file_time_2_utc(&user_time))  
			/ _processor;  
		time = file_time_2_utc(&now);  

		//判断是否为首次计算  
		if ((_last_system_time == 0) || (_last_time == 0))  
		{  
			_last_system_time = system_time;  
			_last_time = time;  
			return -2;  
		}  

		system_time_delta = system_time - _last_system_time;  
		time_delta = time - _last_time;  

		if (time_delta == 0) {  
			return -1;  
		}  

		cpu = (float)system_time_delta * 100 / (float)time_delta;  
		_last_system_time = system_time;  
		_last_time = time;  
		return cpu;  
	} 
}; 