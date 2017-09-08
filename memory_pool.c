#ifdef WIN32
#define cb_lockexchange(k, v) InterlockedExchange(&k, v)
#define cb_lockcompareexchange(k, v, c) InterlockedCompareExchange(&k, v, c)
#define cb_lockexadd(k, v) InterlockedExchangeAdd(&k,  v)
#define cb_lockexsub(k, v) InterlockedExchangeAdd(&k, -v)
#else
#include<pthread>
#define cb_lockexchange(k, v) __sync_val_compare_and_swap(&k, k, v)
#define cb_lockcompareexchange(k, v, c) __sync_val_compare_and_swap(&k, c, v)
#define cb_lockexadd(k, v) __sync_fetch_and_add(&k, v);
#define cb_lockexsub(k, v) __sync_fetch_and_sub(&k, v);
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
			if(!bnewelem)
			{
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
		m_bmap[16]	= 4096;	m_bmap[32]	= 4096;	m_bmap[64]	 = 4096; m_bmap[128]  = 2048; m_bmap[256] = 2048;
		m_bmap[512]	= 1024;	m_bmap[1024]= 1024;	m_bmap[2048] = 1024; m_bmap[4096] = 1024; m_bmap[8192]= 1024;
		//m_bmap[16]	= 100;	m_bmap[32]	= 100;	m_bmap[64]	 = 100; m_bmap[128]  = 100; m_bmap[256] = 100;
		//m_bmap[512]	= 100;	m_bmap[1024]= 100;	m_bmap[2048] = 100; m_bmap[4096] = 100; m_bmap[8192]= 100;
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
	int m_pagesize;
	volatile long m_newnodelock;
	struct node<obj>* m_pnodehead;
	struct node<obj>* m_pnodetail;
	struct node<obj>* m_pnodeaddi;
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
			struct node<obj>* pnode = new_node(iobjcount, m_pagesize);
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
			struct node<obj>* pdel = m_pnodeaddi;
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
				if(m_pagesize >= (sizeof(struct node<obj>) + sizeof(struct node<obj>*) + sizeof(obj)))
				{
					char* pbegin = new(std::nothrow) char[m_pagesize];
					if(pbegin)
					{
						char* pend = pbegin + m_pagesize;
						memset(pbegin, 0, m_pagesize);
						struct node<obj>* pnode = (struct node<obj>*)pbegin;
						pbegin += sizeof(struct node<obj>);
						obj* pobjhead = (obj*)(pbegin + sizeof(struct node<obj>*));
						obj* ptail = pobjhead;
						while(1)
						{
							memcpy(pbegin, &phead, sizeof(struct node<obj>*));
							pbegin += sizeof(struct node<obj>*);
							char* pnext = pbegin + sizeof(obj) + sizeof(struct node<obj>*);
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
				int ilen = sizeof(obj) + sizeof(struct node<obj>*);
				char* p = new(std::nothrow) char[ilen];
				if(!p){
					return 0;
				}
				memset(p, 0, ilen);
				return (obj*)(p + sizeof(struct node<obj>*));
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