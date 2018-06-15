#pragma once
#ifndef __cb_cb__
#define __cb_cb__

//////////////////////////////////include///////////////////////////////////////

#include <map>
#include <vector>

#ifdef WIN32 //iocp
	#include <winsock2.h>
	#include <MSWSock.h>
	#pragma comment(lib, "WS2_32")
#else
	
#endif

//////////////////////////////////include///////////////////////////////////////

//////////////////////////////////public////////////////////////////////////////
typedef volatile unsigned long cb_lock_ul;
typedef void* cb_pvoid;
typedef void* (*pvoid_proc_pvoid)(void*);

#ifdef WIN32
#define cb_api extern"C" __declspec(dllexport)
#define cb_sleep(t) Sleep(t);
#define cb_errno WSAGetLastError()
#define cb_sprintf sprintf_s
#define cb_filename(x) strrchr((x),'\\') ? strrchr((x), '\\') + 1 : (x)
#else
#define cb_api 
#define cb_sleep(t) usleep(t * 1000);
#define cb_errno errno
#define cb_sprintf snprintf
#define cb_filename(x) strrchr((x), '/') ? strrchr((x), '/') + 1 : (x)
#endif
//////////////////////////////////public////////////////////////////////////////

//////////////////////////////////log///////////////////////////////////////////
//cb_log(0, "chenbo", "err", 0, 0, "%d %d", 0, 0);
#ifdef WIN32
#define cb_time(timebuf,timebuflen) {SYSTEMTIME t; GetLocalTime(&t); \
	cb_sprintf(timebuf, timebuflen, "%04d-%02d-%02d %02d:%02d:%02d,%03d", t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,t.wMilliseconds);}
#define cb_log(proc, modulename, level, pbuf, nbuflen, pfmt, ...)	\
{\
	char timebuf[64] = {0}; cb_time(timebuf, 64);\
	if(!pbuf|| nbuflen <= 0)\
	{\
		char buf[4096] = {0};\
		cb_sprintf(buf, sizeof(buf), "%s [%s] [%-16.16s:%5.d] [%s]"pfmt, timebuf, modulename, cb_filename(__FILE__), __LINE__, level, __VA_ARGS__);\
		if(proc)\
			((pvoid_proc_pvoid)proc)(buf);\
		else\
			printf(buf);\
	}\
	else{\
		cb_sprintf(pbuf, nbuflen, "%s [%s] [%-16.16s:%5.d] [%s]"pfmt, timebuf, modulename, cb_filename(__FILE__), __LINE__, level, __VA_ARGS__);\
		if(proc)\
			((pvoid_proc_pvoid)proc)(pbuf);\
		else\
			printf(pbuf);\
	}\
}
#else
#define cb_time(timebuf, timebuflen) {char t[64] = {0}; struct timeval tv; gettimeofday(&tv, NULL); \
	strftime(t, 64, "%Y-%m-%d %H:%M:%S", localtime(&tv.tv_sec));\
	cb_sprintf(timebuf, timebuflen, "%s,%03d",t,(int)(tv.tv_usec / 1000));}
#define cb_log(proc, modulename, level, pbuf, nbuflen, pfmt, ...)	\
{\
	char timebuf[64] = {0}; cb_time(timebuf, 64);\
	if(!pbuf|| nbuflen <= 0)\
	{\
		char ptbuf[4096] = {0};\
		std::string fmt(std::string("%s [%s] [%-16.16s:%5.d] [%s]") + std::string(pfmt));\
		cb_sprintf(ptbuf, 4095, fmt.c_str(), timebuf, modulename, cb_filename(__FILE__), __LINE__, level, ##__VA_ARGS__);\
		if(proc)\
			((pvoid_proc_pvoid)proc)(ptbuf);\
		else\
			printf(ptbuf);\
	}\
	else{\
		std::string fmt(std::string("%s [%s] [%-16.16s:%5.d] [%s]") + std::string(pfmt));\
		cb_sprintf(pbuf, nbuflen, fmt.c_str(), timebuf, modulename, cb_filename(__FILE__), __LINE__, level, ##__VA_ARGS__);\
		if(proc)\
			((pvoid_proc_pvoid)proc)(pbuf);\
		else\
			printf(pbuf);\
	}\
}
#endif
//////////////////////////////////log///////////////////////////////////////////

//////////////////////////////////file//////////////////////////////////////////
#ifdef WIN32
#include <io.h>	//_access
#define cb_fileexist(filepath) !_access((filepath), 0) ? 0 : -1
#define cb_file	FILE*
#define cb_openf(f, filepath, mode) fopen_s(&f, filepath, mode)
#define cb_openf_succ(f) (f)
#define cb_fseek(f, offset, origin)	fseek((f), (offset), (origin))
#define cb_filesizetype	long
#define cb_fileend(filesize, f)	cb_fseek((f), 0, SEEK_END);(filesize) = ftell((f));cb_fseek((f), 0, SEEK_SET)
#define cb_read(f, pbuf, bufsize) fread(pbuf, 1, bufsize, (f))
#define cb_write(f, pbuf, bufsize) fwrite(pbuf, 1, bufsize, (f))
#define cb_closef(f) fclose((f))
#define cb_moderead "rb"
#define cb_modewrite "w+"
#else
#define cb_fileexist(filepath) (!access((filepath), F_OK)) ? 0 : -1
#define cb_file	int
#define cb_openf(f, filepath, mode) (f) = open(filepath, mode)
#define cb_openf_succ(openret) ((openret)!=-1)
#define cb_fseek(f, offset, origin)	lseek((f), (offset), (origin))
#define cb_filesizetype	off_t
#define cb_fileend(filesize, f)	filesize = lseek((f), 0, SEEK_END); lseek((f), 0, SEEK_SET)
#define cb_read(f, pbuf, bufsize) read((f), pbuf, bufsize)
#define cb_write(f, pbuf, bufsize) write((f), pbuf, bufsize)
#define cb_closef(f) close((f))
#define cb_moderead O_RDONLY
#define cb_modewrite O_RDWR|O_CREAT|O_TRUNC
#endif
//////////////////////////////////file//////////////////////////////////////////

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
#pragma comment(lib, "Winmm")	//WinXP -> __imp__timeSetEvent
#include <windows.h>			//for HANDLE
#include <process.h>			//_beginthreadex
#define cb_thread_fd HANDLE
#define cb_thread_create(thread_fd, proc, params) ((thread_fd) = \
	(cb_thread_fd)_beginthreadex(0, 0, (proc), (params), 0, 0))
#define cb_thread_fail(thread_fd) (0 == (thread_fd))
#define cb_thread_close_fd(thread_fd) CloseHandle(thread_fd)
#else
#include <pthread.h>
#define cb_thread_fd pthread_t
#define cb_create_thread(thread_fd, proc, params) pthread_attr_t cb__attr;\
	pthread_attr_init(&cb__attr);\
	pthread_attr_setdetachstate(&cb__attr, 1);\
	pthread_create(&thread_fd, &cb__attr, proc, (void*)params)
#define cb_thread_fail(thread_fd) (0 != (thread_fd))
#define cb_close_thread_fd(thread_fd)
#endif
//////////////////////////////////thread////////////////////////////////////////

//////////////////////////////////socket////////////////////////////////////////
#ifdef WIN32
#pragma comment(lib, "ws2_32.lib")
#define cb_socket SOCKET
#define cb_socket_invalid INVALID_SOCKET
#define cb_sockoptlen int
#define cb_localip(buf, bufsize, __sockaddr_in){\
	char* __pip = inet_ntoa(__sockaddr_in.sin_addr); \
	if(__pip && bufsize > strlen(__pip)) memcpy(buf, __pip, bufsize);}
#define cb_closesock(sock) {if(sock != -1){closesocket(sock);sock = INVALID_SOCKET;}}
#define cb_bool_sock_block (cb_errno == WSAEWOULDBLOCK)
#define cb_ioctlsock ioctlsocket
#define cb_tm_sendrecv(sock, tmsend, tmrecv) {\
	if(tmsend > 0){int nsendtm = tmsend * 1000; setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&nsendtm, sizeof(int));}\
	if(tmrecv > 0){int nrecvtm = tmrecv * 1000; setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&nrecvtm, sizeof(int));}}
#include "Iphlpapi.h"	//SendARP
#pragma comment(lib, "IpHlpApi.lib")	//SendARP
#else
#define cb_socket int
#define cb_socket_invalid -1
#define cb_sockoptlen socklen_t
#define cb_localip(buf, bufsize, __sockaddr_in){\
	inet_ntop(AF_INET, &__sockaddr_in.sin_addr, buf, bufsize);}
#define cb_closesock(sock) {if(sock != -1){close(sock);sock = -1;}}
#define cb_bool_sock_block (cb_errno == EINPROGRESS)
#define cb_ioctlsock ioctl
#define cb_tm_sendrecv(sock, tmsend, tmrecv) {struct timeval ti;ti.tv_usec = 0;\
	if(tmsend > 0){ti.tv_sec = tmsend; setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&ti, sizeof(ti));}\
	if(tmrecv > 0){ti.tv_sec = tmrecv; setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&ti, sizeof(ti));}}
#endif
//////////////////////////////////socket////////////////////////////////////////

namespace cb_space_publicfunc
{
	struct cb_timeinfo{
		cb_timeinfo():year(0), month(0), day(0), hour(0), minute(0), second(0), millisecond(0){}
		int year;
		int month;
		int day;
		int hour;
		int minute;
		int second;
		int millisecond;
	};
	class cb_staticfunc
	{
	public:
		static void wstring2string(std::string& dest, const std::wstring& src)
		{
			if(src.length() <= 0)
				return ;
#ifdef WIN32
			int ilen = ::WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, 0, 0, 0, 0) + 1;
			char buf[4096] = {0}; char* p = buf;
			if(ilen >= 4096)
			{
				p = new(std::nothrow) char[ilen];
				if(!p)
					return ;
				memset(p, 0, ilen * sizeof(char));
			}
			::WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, p, ilen - 1, 0, 0);
			dest = p;
			if(p != buf)
				delete []p, p = 0;
			return ;
#else
			/*
			std::string curlocale = setlocale(LC_ALL, 0); // curLocale = "C";
			setlocale(LC_ALL, "chs");
			size_t ilen = src.size() * sizeof(wchar_t) + 1;
			char buf[4096] = {0}; char* p = buf;
			if(ilen >= 4096)
			{
				p = new(std::nothrow) char[ilen];
				if(!p)
					goto lab_end;
				memset(p, 0, ilen * sizeof(char));
			}
			wcstombs(p, src.c_str(), ilen);
			dest = p;
			if(p != buf)
				delete []p, p = 0;
lab_end:
			setlocale(LC_ALL, curLocale.c_str());
			*/
#endif
		}

		static void string2wstring(std::wstring& dest, const std::string& src)
		{
			if(src.length() <= 0)
				return ;
#ifdef WIN32
			int ilen = ::MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, 0, 0) + 1;
			wchar_t buf[4096] = {0}; wchar_t* p = buf;
			if(ilen >= 4096)
			{
				p = new(std::nothrow) wchar_t[ilen];
				if(!p)
					return ;
				memset(p, 0, ilen * sizeof(wchar_t));
			}
			::MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, p, ilen - 1);
			dest = p;
			if(p != buf)
				delete []p, p = 0;
#else
			/*
			setlocale(LC_ALL, "chs");
			size_t ilen = src.size() + 1;
			char buf[4096] = {0}; char* p = buf;
			if(ilen >= 4096)
			{
				p = new(std::nothrow) char[ilen];
				if(!p)
					goto lab_end;
				memset(p, 0, ilen * sizeof(char));
			}
			mbstowcs(p, src.c_str(), ilen);
			dest = p;
			if(p != buf)
				delete []p, p = 0;
lab_end:
			setlocale(LC_ALL, "C");
			*/
#endif
		}

		static int readfile(const char* filepath, char** pbuf, int& bufsize)
		{
			if(!filepath || cb_fileexist(filepath)){
				return -1;
			}
			cb_file f; cb_openf(f, filepath, cb_moderead);
			if(!cb_openf_succ(f)){
				return -2;
			}
			cb_filesizetype filesize;
			cb_fileend(filesize, f);
			if(filesize == 0){
				cb_closef(f);
				return 0;
			}
			else if(filesize < 0){
				cb_closef(f);
				return -3;
			}
			if(bufsize < filesize){
				*pbuf = new char[filesize];
				if(!*pbuf){
					cb_closef(f);
					return -4;
				}
				memset(*pbuf, 0, filesize);
			}
			bufsize = filesize;
			long nidx = filesize; int iret = 0;
			while(filesize > 0)
			{
				if(cb_fseek(f, (nidx - filesize), SEEK_SET)){
					iret = 1;
					break;
				}
				long icount = cb_read(f, &(*pbuf)[nidx - filesize], filesize);
				if(icount <= 0){
					iret = 2;
					break;
				}
				filesize -= icount;
			}
			cb_closef(f);
			return iret;
		}

		static int writefile(const char* filepath, const char* pbuf, int bufsize)
		{
			if(!filepath || cb_fileexist(filepath)){
				return -1;
			}
			cb_file f; cb_openf(f, filepath, cb_modewrite);
			if(!cb_openf_succ(f)){
				return -2;
			}
			long nidx(bufsize); int iret = 0;
			while(bufsize > 0){
				if(cb_fseek(f, (nidx - bufsize), SEEK_SET)){
					iret = 1;
					break;
				}
				long icount = (cb_write(f, &pbuf[nidx - bufsize], bufsize));
				if(icount <= 0){
					iret = 2;
					break;
				}
				bufsize -= icount;
			}
			cb_closef(f);
			return 0;
		}

		static int istextutf8(char* str, int istrlen)
		{
			unsigned long nbytes = 0;
			bool ballascii = true;
			for(int i = 0; i < istrlen; ++i)
			{
				if((str[i] & 0x80) != 0){
					ballascii = false;
				}
				if(nbytes == 0)
				{
					if(str[i] >= 0x80)
					{
						if(str[i] >= 0xFC && str[i] <= 0xFD)
							nbytes = 6;
						else if(str[i] >= 0xF8)
							nbytes = 5;
						else if(str[i] >= 0xF0)
							nbytes = 4;
						else if(str[i] >= 0xE0)
							nbytes = 3;
						else if(str[i] >= 0xC0)
							nbytes = 2;
						else
							return -1;
						--nbytes;
					}
				}
				else{
					if((str[i] & 0xC0) != 0x80){
						return -2;
					}
					--nbytes;
				}
			}
			if(nbytes > 0){
				return -3;
			}
			if(ballascii){
				return -4;
			}
			return 0;
		}
		
	#ifdef WIN32
		/*
		#ifdef WIN32
		#define cb_transcoding_read  CP_UTF8, CP_ACP
		#define cb_transcoding_write CP_ACP, CP_UTF8
		#else
		#define cb_transcoding_read  "utf-8", "gbk"
		#define cb_transcoding_write "gbk", "utf-8"
		#endif
		transcoding(str, sText.c_str(), cb_codeconvert_value_write);*/
		static int transcoding(std::string& out, const char* _in, unsigned int fromcodepage, unsigned int tocodepage)
	#else
		static int transcoding(std::string& out, const char* _in, const char* fromcodepage, const char* tocodepage)
	#endif
		{
			if(!_in || !*_in){
				return -1;
			}
	#ifdef WIN32
			int len = MultiByteToWideChar(fromcodepage, 0, _in, -1, NULL, 0);
			if(len <= 0){
				return -2;
			}
			wchar_t* pwcharbuf = new(std::nothrow) wchar_t[len + 1];
			if(!pwcharbuf){
				return -3;
			}
			memset(pwcharbuf, 0, sizeof(wchar_t) * (len + 1));
			MultiByteToWideChar(fromcodepage, 0, _in, -1, pwcharbuf, len);
			len = WideCharToMultiByte(tocodepage, 0, pwcharbuf, -1, NULL, 0, NULL, NULL);
			if(len <= 0){
				delete []pwcharbuf, pwcharbuf = 0;
				return -4;
			}
			char* pcharbuf = new(std::nothrow) char[len + 1];
			if(!pcharbuf){
				delete []pwcharbuf, pwcharbuf = 0;
				return -5;
			}
			memset(pcharbuf, 0, sizeof(char) * (len + 1));
			WideCharToMultiByte(tocodepage, 0, pwcharbuf, -1, pcharbuf, len, NULL, NULL);
			out = pcharbuf;
			delete []pcharbuf, pcharbuf = 0;
	#else
			int inlen = strlen(_in);
			if(inlen <= 0){
				return -2;
			}
			size_t outlen_t = inlen * 4 + 1;
			char* pcharbuf = new(std::nothrow) char[outlen_t];
			if(!pcharbuf){
				return -3;
			}
			memset(pcharbuf, 0, outlen_t);
			iconv_t cd = iconv_open(tocodepage, fromcodepage);
			if(cd == (iconv_t)(-1)){
				delete[] pcharbuf, pcharbuf = 0;
				return -4;
			}
			char* pin = (char*)_in;
			char* pout= pcharbuf;
			size_t inlen_t = inlen;
			if(iconv(cd, &pin, &inlen_t, &pout, &outlen_t) == (size_t)(-1)){
				iconv_close(cd);
				delete[] pcharbuf, pcharbuf = 0;
				return -5;
			}
			iconv_close(cd);
			out = pcharbuf;
			delete []pcharbuf, pcharbuf = 0;
	#endif
			return 0;
		}
		
		/*
		cb_timeinfo tinfo;
		cb_space_publicfunc::cb_staticfunc::gettimeinfo(tinfo);
		printf("%d-%02d-%02d %02d:%02d:%02d %03d\n", 
			tinfo.year, tinfo.month, tinfo.day, tinfo.hour, tinfo.minute, tinfo.second, tinfo.millisecond);*/
		static int gettimeinfo(cb_timeinfo& tinfo)
		{
#ifdef WIN32
			SYSTEMTIME t;
			GetLocalTime(&t);
			tinfo.year = t.wYear; tinfo.month = t.wMonth; tinfo.day = t.wDay;
			tinfo.hour = t.wHour; tinfo.minute = t.wMinute; tinfo.second = t.wSecond; tinfo.millisecond = t.wMilliseconds;
#else
			struct timeval tv;
			memset(&tv, 0, sizeof(timeval));
			gettimeofday(&tv, NULL);
			struct tm* time_ptr = localtime(&tv.tv_sec);
			if(!time_ptr)
				return -1;
			tinfo.year = time_ptr->tm_year + 1900; tinfo.month = time_ptr->tm_mon + 1; tinfo.day = time_ptr->tm_mday;
			tinfo.hour = time_ptr->tm_hour; tinfo.minute = time_ptr->tm_min; tinfo.second = time_ptr->tm_sec; tinfo.millisecond = tv.tv_usec / 1000;
#endif
			return 0;
		}
	};
};
namespace cb_space_memorypool
{
	class mem_pool
	{
#define __mem_pool_offset__ sizeof(struct mcb_node*)
		struct mcb_node
		{
		public:
			mcb_node():m_nlock(0), m_pmemhead(0), m_ntlock(0), m_pmemt(0), m_ielemsize(0), m_pblock(0), m_pnextnode(0){}
		public:
			cb_lock_ul m_nlock;			//连续内存单元锁
			void* m_pmemhead;				//连续内存单元的第一个地址,每个单元头sizeof(void*)个字节内容指向下一个内存单元的地址
			cb_lock_ul m_ntlock;			//临时内存单元锁
			void* m_pmemt;					//临时存储一个变量,可提高效率
			unsigned int m_ielemsize;		//每个内存块的size
			void* m_pblock;					//内存块地址,后sizeof(void*)个字节内容指向下一个内存块的地址,但是内存块都插入到m_pmemhead
			mcb_node* m_pnextnode;	//指向下个node
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
			void* new_elem(mcb_node* pnode, unsigned int ielemcount = 1024, bool bnewelem = false, void** ptail = 0)
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
						memset(p, 0, sizeof(mcb_node*));
						p += sizeof(mcb_node*);//0 flag for system delete
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
				void* phead = p + sizeof(mcb_node*);
				while(1)
				{
					//node add
					memcpy(p, &pnode, sizeof(mcb_node*));
					p += sizeof(mcb_node*);
					*ptail = p;//set last elem for insert list
					//elemnext
					char* pnext = p + ielemsize + sizeof(mcb_node*);
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
			mcb_node* pnodehead = 0;
			int i = sizeof(arr)/sizeof(arr[0]) - 1;
			for(; i >= 0; --i)
			{
				mcb_node* pnode = new_node(arr[i], m_bmap[arr[i]]);
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
			mcb_node* phead = m_pmemorydelhead;
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
		void* mcb_pool_new(unsigned int n, int iflag = 1)
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
			char* p = new(std::nothrow)char[n + sizeof(mcb_node*)];
			if(p){
				memset(p, 0, sizeof(mcb_node*));
				p += sizeof(mcb_node*);
			}
			return p;
		}
		void  mcb_pool_del(void* p)
		{
			if(!p) return ;
			mcb_node* pnode = 0;
			char* pdel = (char*)p - sizeof(mcb_node*);
			memcpy(&pnode, pdel, sizeof(mcb_node*));
			pnode ? pnode->del_mem(p) : delete pdel, pdel = 0;
		}
	private:
		mcb_node* new_node(unsigned int ielemsize, unsigned int ielemcount)
		{
			unsigned int size = ielemsize * ielemcount;
			char* p = new(std::nothrow)char[size];
			if(!p || (size < sizeof(mcb_node) + sizeof(mcb_node*) + ielemsize)){
				if(p){
					delete p, p = 0;
				}
				return 0;
			}
			char* pend = p + size;
			memset(p, 0, size);
			mcb_node* pnode = (mcb_node*)p;
			pnode->m_ielemsize = ielemsize;
			p += sizeof(mcb_node);
			pnode->m_pmemt = p + sizeof(mcb_node*);
			while(1){
				memcpy(p, &pnode, sizeof(mcb_node*));//node add
				p += sizeof(mcb_node*);
				//elemnext
				char* pnext = p + ielemsize + sizeof(mcb_node*);
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
		mcb_node* m_pmemorydelhead;
		mcb_node* m_p__16; mcb_node* m_p__32;
		mcb_node* m_p__64; mcb_node* m_p_128;
		mcb_node* m_p_256; mcb_node* m_p_512;
		mcb_node* m_p1024; mcb_node* m_p2048;
		mcb_node* m_p4096; mcb_node* m_p8192;
	};
	/*
	struct memtest
	{
		memtest():next(0), prev(0){}
		memtest* next;
		memtest* prev;
		int i;
		char x;
		double d;
	};
	
	memtest* head = 0;
	volatile unsigned long lock = 0;
	int add(memtest* pdata)
	{
		if(!pdata)
			return -1;
		while(cb_lockcompareexchange(lock, 1, 0) == 1);
		if(!head){
			head = pdata;
		}
		else{
			pdata->next = head;
			head->prev = pdata;
			head = pdata;
		}
		cb_lockexchange(lock, 0);
		return 0;
	}
	
	memtest* get(void)
	{
		memtest* p = 0;
		while(cb_lockcompareexchange(lock, 1, 0) == 1);
		if(head){
			p = head;
			head = head->next;
			if(head)
				head->prev = 0;
			p->next = 0;
		}
		cb_lockexchange(lock, 0);
		return p;
	}
	
	static unsigned int __stdcall memtestproc(void* p)
	{
		cb_space_memorypool::obj_pool<memtest>* pmemt = (cb_space_memorypool::obj_pool<memtest>*)p;
		while(1){
			memtest* pt = pmemt->newobj();
			if(pt){
				add(pt);
				//cb_sleep(1);
			}
		}
		return 0;
	}
	
	static unsigned int __stdcall memtestproc2(void* p)
	{
		cb_space_memorypool::obj_pool<memtest>* pmemt = (cb_space_memorypool::obj_pool<memtest>*)p;
		while(1){
			memtest* pt = get();
			if(pt){
				pmemt->delobj(pt);
				//cb_sleep(1);
			}
		}
		return 0;
	}

	cb_space_memorypool::obj_pool<memtest> memt;
	for(int i = 0; i < 200; ++i){
		cb_thread_fd tfd;
		cb_create_thread(tfd, memtestproc, &memt);
		cb_thread_fail(tfd);
	}

	for(int i = 0; i < 100; ++i){
		cb_thread_fd tfd;
		cb_create_thread(tfd, memtestproc2, &memt);
		cb_thread_fail(tfd);
	}
	*/
	template<typename obj>class obj_pool
	{
		template<typename elem> struct node
		{
			node():m_nlock(0), m_pelemhead(0), m_pnodenext(0){}
			cb_lock_ul		m_nlock;
			elem*				m_pelemhead;
			node<elem>*	m_pnodenext;
		};
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
	private:
		int m_pagesize;
		cb_lock_ul m_newnodelock;
		node<obj>* m_pnodehead;
		node<obj>* m_pnodetail;
		node<obj>* m_pnodeaddi;
		cb_lock_ul m_naddilock;
		obj* m_pobj;
		cb_lock_ul m_nobjlock;
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
					cb_sleep(200);
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

namespace cb_space_polling
{
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
template<typename t> class cb_timerpoling
{
	struct pollnode
	{
		cb_lock_ul m_nlock;
		t* m_pthead;
		pollnode* m_pprev;
		pollnode* m_pnext;
	};
	struct cb_params{
		pvoid_proc_pvoid proc;
		cb_timerpoling* pthis;
	};
	cb_timerpoling(const cb_timerpoling& ref);
	cb_timerpoling& operator=(const cb_timerpoling& ref);
public:
	cb_timerpoling():m_phead(0),m_pdel(0){}
	virtual ~cb_timerpoling()
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

class cb_timerpolling2;
struct pollnodeex;
struct telem
{
	telem():pprev(0),pnext(0),ppollnodeex(0){}
	telem* pprev;
	telem* pnext;
	pollnodeex* ppollnodeex;
};
struct pollnodeex
{
	pollnodeex():m_nlock(0),m_pthead(0),m_pprev(0),m_pnext(0){}
	cb_lock_ul	m_nlock;
	telem*		m_pthead;
	pollnodeex* m_pprev;
	pollnodeex* m_pnext;
};
struct cb_paramsex
{
	cb_paramsex():proc(0),pthis(0){}
	pvoid_proc_pvoid proc;
	cb_timerpolling2* pthis;
};
#ifndef WIN32
void* __cb_poling_proc__(void* pparams);
#endif
/*
struct cb2 : public cb_space_polling::telem{
	long x;
};

void* proc2(void* p)
{
	if(!p) return 0;
	cb2* pt = (cb2*)p;
	cb_log(0, "chenbo", "err", 0, 0, "%04d\n", pt->x);
	return 0;
}

cb_space_memorypool::obj_pool<cb2> memt;
cb_space_polling::cb_timerpolingex t;
t.start(1000, 10, proc2, 10);
void* p = 0;
int i = 0;
while(1){
	if(i++ > 1000000){
		i = 0;
	}
	cb2* pt = memt.newobj();
	pt->x = i;
	Sleep(10);
	t.add(pt);
	if(i%7 == 0){
		printf("del %d\n", i);
		t.del(pt);
	}
}
getchar();
t.stop();
*/
class cb_timerpolling2
{
#ifndef WIN32
	friend void* __cb_poling_proc__(void* pparams);
#endif
public:
	cb_timerpolling2()
		:m_handlehead(0),m_handlelock(0),m_nhandleprocexitflag(0),m_nhandleproccount(0),m_phead(0),m_pdel(0){}
	virtual ~cb_timerpolling2(){stop();}
public:
	int start(int idelaytime, int ipolingcount, pvoid_proc_pvoid proc, int iproccount = 1)
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
		while(iproccount-- > 0)
		{
			cb_thread_fd tfd;
#ifdef WIN32
			cb_thread_create(tfd, __cb_poling_proc__, &m_paramsex);
#else
			cb_thread_create(tfd, __cb_poling_proc__, &m_paramsex);
#endif
			if(cb_thread_fail(tfd)){
				return -3;
			}
			cb_lockexadd(m_nhandleproccount, 1);
		}
		return 0;
	}
	int stop(void)
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
		//timer
		m_timer.stop();
		//delete memory
		if(m_pdel){
			delete []m_pdel, m_pdel = 0;
		}
		return 0;
	}
	int add(telem* pt)
	{
		if(!pt) return -1;
		pollnodeex* padd = m_phead->m_pprev;
		while(cb_lockcompareexchange(padd->m_nlock, 1, 0) == 1) padd = m_phead->m_pprev;
		{
			pt->ppollnodeex = padd;
			if(padd->m_pthead)
			{
				padd->m_pthead->pprev = pt;
				pt->pnext = padd->m_pthead;
			}
			padd->m_pthead = pt;
		}
		cb_lockexchange(padd->m_nlock, 0);
		return 0;
	}
	int del(void* p)
	{
		if(!p) return -1;
		telem* pt = (telem*)p;
		if(pt->ppollnodeex)
		{
			pollnodeex* pnode = pt->ppollnodeex;
			while(cb_lockcompareexchange(pnode->m_nlock, 1, 0) == 1);
			{
				if(!pt->pprev){
					pnode->m_pthead = pt->pnext;
					if(pt->pnext){
						pt->pnext->pprev = 0;
					}
				}
				else{
					pt->pprev->pnext = pt->pnext;
					if(pt->pnext){
						pt->pnext->pprev = pt->pprev;
					}
				}
				pt->ppollnodeex = 0;
				pt->pnext = pt->pprev = 0;
			}
			cb_lockexchange(pnode->m_nlock, 0);
		}
		return 0;
	}
private:
	telem* get(void)
	{
		telem* pret = 0;
		while(cb_lockcompareexchange(m_handlelock, 1, 0) == 1);
		if(m_handlehead)
		{
			pret = m_handlehead;
			m_handlehead = m_handlehead->pnext;
			if(pret->pnext)
				pret->pnext->pprev = 0;
			pret->pnext = 0;
		}
		cb_lockexchange(m_handlelock, 0);
		return pret;
	}
	int invokeex(void)
	{
		while(cb_lockcompareexchange(m_phead->m_nlock, 1, 0) == 1);
		pollnodeex* pdelnode = m_phead;
		m_phead = m_phead->m_pnext;
		if(pdelnode->m_pthead)
		{
			telem* ptelemhead = pdelnode->m_pthead;
			telem* ptelemtail = 0;
			while(ptelemhead)
			{
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
	void add(telem* phead, telem* ptail)
	{
		if(!phead) return ;
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
	void releasehandletelem(void)
	{
		if(m_handlehead)
		{
			while(cb_lockcompareexchange(m_handlelock, 1, 0) == 1);
			telem* pdel = m_handlehead;
			while(pdel)
			{
				m_handlehead = m_handlehead->pnext;
				if(m_handlehead){
					m_handlehead->pprev = 0;
				}
				pdel->pnext = 0;
				pdel->ppollnodeex = 0;
				m_paramsex.proc(pdel);
				pdel = m_handlehead;
			}
			m_handlehead = 0;
			cb_lockexchange(m_handlelock, 0);
		}
	}
	static void* invokeex(void* pparams)
	{
		cb_timerpolling2* pthis = (cb_timerpolling2*)pparams;
		if(!pthis){
			return (void*)-1;
		}
		pthis->invokeex();
		return 0;
	}
#ifdef WIN32
	static unsigned int __stdcall __cb_poling_proc__(void* pparams);
#endif
	pollnodeex* createloop(int ipolingcount)
	{
		if(ipolingcount <= 0)
			return 0;
		ipolingcount += 1;//
		m_pdel = new(std::nothrow) pollnodeex[ipolingcount];
		if(!m_pdel)
			return 0;
		memset(m_pdel, 0, sizeof(pollnodeex) * ipolingcount);
		pollnodeex* ptail = 0;
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
public:
	cb_lock_ul m_nhandleprocexitflag;
	cb_lock_ul m_nhandleproccount;
private:
	telem* m_handlehead;
	cb_lock_ul m_handlelock;
	pollnodeex* m_phead;
	pollnodeex* m_pdel;
	cb_paramsex m_paramsex;
	cb_space_timer::cb_timer m_timer;
};
#ifndef WIN32
void* __cb_poling_proc__(void* pparams)
#else
unsigned int __stdcall cb_timerpolling2::__cb_poling_proc__(void* pparams)
#endif
{
	cb_paramsex* p = (cb_paramsex*)pparams;
	if(!p)
#ifdef WIN32
		return -1;
#else
		return (void*)-1;
#endif
	int iret = 0;
	cb_timerpolling2* pthis = p->pthis;
	while(1)
	{
		if(cb_lockcompareexchange(pthis->m_nhandleprocexitflag, 1, 1) == 1)
		{
			cb_lockexsub(pthis->m_nhandleproccount, 1);
			break;
		}
		telem* ptelem = pthis->get();
		if(!ptelem){
			cb_sleep(10);
		}
		else{
			if(p->proc){
				p->proc(ptelem);
			}
			else{
				cb_lockexsub(pthis->m_nhandleproccount, 1);
				break;
			}
		}
	}
	return 0;
}

/*
struct cb : public cb_space_poling::serverbase{
	int x;
};
cb_space_memorypool::obj_pool<cb> cb_pool;
void* proc2(void* p)
{
	if(p){
		cb* pcb = (cb*)p;
		//cb_log(0, "chenbo", "info", 0, 0, "timeout %04d\n", pcb->x);
		cb_pool.delobj(p);
	}
	return 0;
}
cb_space_polling::cb_timerpollingserver s;
s.start(1000, 1, proc2);
for(int i = 0; i < 100000; ++i){
	cb* pz = cb_pool.newobj();
	if(pz){
		pz->x = i;
		s.add(pz);
		if(i%9 == 0){
			s.del(pz);
			//cb_log(0, "chenbo", "info", 0, 0, "del %04d\n", pz->x);
			cb_pool.delobj(pz);
		}
	}
}
getchar();
s.stop();
*/
struct pollingnodeex;
struct cb_base
{
	cb_base():m_nlock(0),m_pprev(0),m_pnext(0){}
	cb_lock_ul m_nlock;
	cb_base* m_pprev;
	cb_base* m_pnext;
	pollingnodeex* m_pnode;
};
struct pollingnodeex
{
	pollingnodeex():m_pbase(0),m_nlock(0),m_pprev(0),m_pnext(0){}
	cb_base* m_pbase;
	cb_lock_ul m_nlock;
	pollingnodeex* m_pprev;
	pollingnodeex* m_pnext;
};
class cb_timerpollingex
{
public:
	cb_timerpollingex():m_proc(0),m_pnodehead(0),m_pdel(0){}
	virtual ~cb_timerpollingex(){stop();}
public:
	static void* invoke(void* pparams)
	{
		cb_timerpollingex* pthis = (cb_timerpollingex*)pparams;
		pthis->invoke();
		return 0;
	}
public:
	int start(int idelaytime, int ipolingcount, pvoid_proc_pvoid _proc)
	{
		if(idelaytime <= 0 || ipolingcount <= 0 || !_proc){
			return -1;
		}
		if(!createloop(ipolingcount)){
			return -2;
		}
		m_proc = _proc;
		return m_timer.start(idelaytime/ipolingcount, invoke, this);
	}
	int add(cb_base* padd)
	{
		if(!padd) return -1;
		pollingnodeex* pnode = m_pnodehead->m_pprev;
		while(cb_lockcompareexchange(pnode->m_nlock, 1, 0) == 1) pnode = m_pnodehead->m_pprev;
		{
			while(cb_lockcompareexchange(padd->m_nlock, 1, 0) == 1);
			{
				if(pnode->m_pbase){
					pnode->m_pbase->m_pprev = padd;
					padd->m_pnext = pnode->m_pbase;
				}
				padd->m_pnode = pnode;
				pnode->m_pbase = padd;
			}
			cb_lockexchange(padd->m_nlock, 0);
		}
		cb_lockexchange(pnode->m_nlock, 0);
		return 0;
	}
	int del(cb_base* pdel)
	{
		if(!pdel) return -1;
		if(pdel->m_pnode)
		{
			pollingnodeex *pnode = pdel->m_pnode;
			while(cb_lockcompareexchange(pnode->m_nlock, 1, 0) == 1);
			{
				while(cb_lockcompareexchange(pdel->m_nlock, 1, 0) == 1);
				{
					if(!pdel->m_pprev)
					{
						pnode->m_pbase = pdel->m_pnext;
						if(pdel->m_pnext)
							pdel->m_pnext->m_pprev = 0;
					}
					else{
						pdel->m_pprev->m_pnext = pdel->m_pnext;
						if(pdel->m_pnext)
							pdel->m_pnext->m_pprev = pdel->m_pprev;
					}
					pdel->m_pnode = 0;
					pdel->m_pnext = pdel->m_pprev = 0;
				}
				cb_lockexchange(pdel->m_nlock, 0);
			}
			cb_lockexchange(pnode->m_nlock, 0);
		}
		return 0;
	}
	int stop(void)
	{
		m_timer.stop();
		m_proc = 0; m_pnodehead = 0;
		if(m_pdel){
			delete []m_pdel, m_pdel = 0;
		}
		return 0;
	}
private:
	int invoke(void)
	{
		while(cb_lockcompareexchange(m_pnodehead->m_nlock, 1, 0) == 1);
		{
			cb_base* phead = m_pnodehead->m_pbase;
			while(phead)
			{
				cb_base* pdel = phead;
				while(cb_lockcompareexchange(pdel->m_nlock, 1, 0) == 1);
				phead = phead->m_pnext;
				if(phead)
					phead->m_pprev = 0;
				pdel->m_pprev = 0;
				pdel->m_pnode = 0;
				pdel->m_pnext = 0;
				cb_lockexchange(pdel->m_nlock, 0);
				m_proc(pdel);
			}
			m_pnodehead->m_pbase = 0;
			pollingnodeex* pnode = m_pnodehead;
			m_pnodehead = m_pnodehead->m_pnext;
			cb_lockexchange(pnode->m_nlock, 0);
		}
		return 0;
	}
	pollingnodeex* createloop(int ipolingcount)
	{
		if(ipolingcount <= 0)
			return 0;
		ipolingcount += 1;//
		m_pdel = new(std::nothrow) pollingnodeex[ipolingcount];
		if(!m_pdel)
			return 0;
		memset(m_pdel, 0, sizeof(pollingnodeex) * ipolingcount);
		pollingnodeex* ptail = 0;
		for(int i = 0; i < ipolingcount - 1; ++i)
		{
			m_pnodehead = &m_pdel[i]; ptail = &m_pdel[i + 1];
			m_pnodehead->m_pnext = ptail;
			ptail->m_pprev = m_pnodehead;
		}
		ptail->m_pnext = m_pdel;
		m_pdel->m_pprev = ptail;
		m_pnodehead = m_pdel;
		return m_pnodehead;
	}
private:
	pvoid_proc_pvoid m_proc;
	pollingnodeex* m_pnodehead;
	pollingnodeex* m_pdel;
	cb_space_timer::cb_timer m_timer;
};
};

namespace cb_space_socket
{
	class cb_csocket
	{
	public:
		cb_csocket():m_sock(cb_socket_invalid){}
		virtual ~cb_csocket(){cb_closesock(m_sock);}
	public:
		int connectserver(const std::string& ip, u_short port, int itimeout = 30)
		{
			sockaddr_in saddr;
			memset(&saddr, 0, sizeof(sockaddr_in));
			saddr.sin_family			= AF_INET;
			saddr.sin_port				= htons((u_short)port);
			if((saddr.sin_addr.s_addr	= inet_addr(ip.c_str())) == -1){
				return -1;
			}
			if((m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
				return -2;
			}
			u_long non_blocking = 1;//non block
			if(cb_ioctlsock(m_sock, FIONBIO, &non_blocking)){
				cb_closesock(m_sock);
				return -3;
			}
			switch(connect(m_sock, (sockaddr*)&saddr, sizeof(saddr)))
			{
			case 0:
				{
					u_long blocking = 0;
					if(cb_ioctlsock(m_sock, FIONBIO, &blocking)){
						cb_closesock(m_sock);
						return -4;
					}
				}
				return 0;// c/s in the same computer
			case -1:
				if(cb_bool_sock_block){
					struct timeval tv;
					tv.tv_sec	= itimeout; tv.tv_usec	= 0;
					fd_set writefds; FD_ZERO(&writefds); FD_SET(m_sock, &writefds);
					int selret = select(m_sock + 1, 0, &writefds, 0, &tv);//windows ignore first param
					if(selret < 0){
						cb_closesock(m_sock);
						return -5;
					}
					else if(!selret){
						cb_closesock(m_sock);
						return -6;
					}
					else{
						if(FD_ISSET(m_sock, &writefds)){
							int error = 0;
							cb_sockoptlen optLen = sizeof(error);
							if(getsockopt(m_sock, SOL_SOCKET, SO_ERROR, (char*)&error, &optLen) == -1){
								cb_closesock(m_sock);
								return -7;
							}
							else{
								u_long blocking = 0;
								if(cb_ioctlsock(m_sock, FIONBIO, &blocking)){
									cb_closesock(m_sock);
									return -8;
								}
								return 0;
							}
						}
						else{
							cb_closesock(m_sock);
							return -9;
						}
					}
				}
				else{
					cb_closesock(m_sock);
					return -10;
				}
			default:
				cb_closesock(m_sock);
				return -11;
			}
			return -12;
		}
		int closesock(void)
		{
			cb_closesock(m_sock);
			return 0;
		}
		int senddata(char* psendbuf, int nsendsize, int tvsec = 30 * 1000)
		{
			if(!psendbuf || nsendsize <= 0){
				return -1;
			}
			int nsendcount = 0;
			int iret = 0;
			while(1)
			{
				fd_set wfds; FD_ZERO(&wfds); FD_SET(m_sock, &wfds);
				struct timeval tv; tv.tv_sec = tvsec / 1000; tv.tv_usec = tvsec % 1000;
				int selret = select(m_sock + 1, 0, &wfds, 0, tvsec > 0 ? &tv : 0);//windows ignore first param
				if(selret < 0){
					iret = -1;
					break;
				}
				else if(!selret){
					iret = -2;
					break;
				}
				if(!FD_ISSET(m_sock, &wfds)){
					iret = -3;
					break;
				}
				int error = 0;
				cb_sockoptlen optLen = sizeof(error);
				if(getsockopt(m_sock, SOL_SOCKET, SO_ERROR, (char*)&error, &optLen) == -1){
					iret = -4;
					break;
				}
				int nsendret = send(m_sock, psendbuf + nsendcount, nsendsize - nsendcount, 0);
				if(nsendret > 0){
					if((nsendcount += nsendret) >= nsendsize){
						return 0;
					}
				}
				else if(nsendret < 0){
					iret = -5;
					break;
				}
				else{
					iret = -6;
					break;
				}
			}
			return iret;
		}
		int recvdata(char* pbuf, int ibuflen, int tvsec = 30 * 1000)
		{
			if(!pbuf || ibuflen <= 0){
				return -1;
			}
			int iret = 0;
			int nrecvheadcount = 0;
			while(1)
			{
				fd_set rfds; FD_ZERO(&rfds); FD_SET(m_sock, &rfds);
				struct timeval tv; tv.tv_sec = tvsec / 1000; tv.tv_usec = tvsec % 1000;
				int selret = select(m_sock + 1, &rfds, 0, 0, tvsec > 0 ? &tv : 0);
				if(selret < 0){
					iret = -1; break;
				}
				else if(!selret){
					iret = -2; break;
				}
				if(!FD_ISSET(m_sock, &rfds)){
					iret = -3; break;
				}
				int error = 0;
				cb_sockoptlen optLen = sizeof(error);
				int nerr = getsockopt(m_sock, SOL_SOCKET, SO_ERROR, (char*)&error, &optLen);
				if(nerr == -1){
					iret = -4; break;
				}
				int nrecvret = recv(m_sock, pbuf + nrecvheadcount, ibuflen - nrecvheadcount, 0);
				if(nrecvret > 0)
				{
					if((nrecvheadcount += nrecvret) >= ibuflen){
						break;
					}
				}
				else{
					iret = -5; break;
				}
			}
			return iret;
		}
	public:
		int testnetspeed(const std::string& ip, u_short port, 
			int itestcount = 3, int itestspace = 0 * 1000, char* psendbuf = 0, int isendlen = 0, int iretheadsize = 0)
		{
			if(itestcount <= 0 || ip.empty()){
				return -1;
			}
			while(itestcount-- > 0)
			{
				if(0 == connectserver(ip, port))
				{
					if(psendbuf && isendlen > 0 && iretheadsize > 0)
					{
						if(0 == senddata(psendbuf, isendlen))
						{
							char head[1024] = {0}; char* phead = head;
							if(iretheadsize > 1024)
							{
								phead = new(std::nothrow) char[iretheadsize];
								if(!phead){
									return -2;
								}
								memset(phead, 0, iretheadsize);
							}
							recvdata(phead, iretheadsize);
							if(head != phead){
								delete phead, phead = 0;
							}
						}
					}
					closesock();
					if(itestspace > 0){
						cb_sleep(itestspace);
					}
				}
				else{
					return -3;
				}
			}
			return 0;
		}
	private:
		cb_socket m_sock;
	};

	class cb_csocketex
	{
	public:
		cb_csocketex():m_sock(cb_socket_invalid){}
		virtual ~cb_csocketex(){}
	public:

	private:
		cb_socket m_sock;
	};
};

namespace cb_space_endecryption
{
#pragma region	aes
	const unsigned char aes_sbox[16][16] = 
	{
		{0x63,0x7C,0x77,0x7B,0xF2,0x6B,0x6F,0xC5,0x30,0x01,0x67,0x2B,0xFE,0xD7,0xAB,0x76},
		{0xCA,0x82,0xC9,0x7D,0xFA,0x59,0x47,0xF0,0xAD,0xD4,0xA2,0xAF,0x9C,0xA4,0x72,0xC0},
		{0xB7,0xFD,0x93,0x26,0x36,0x3F,0xF7,0xCC,0x34,0xA5,0xE5,0xF1,0x71,0xD8,0x31,0x15},
		{0x04,0xC7,0x23,0xC3,0x18,0x96,0x05,0x9A,0x07,0x12,0x80,0xE2,0xEB,0x27,0xB2,0x75},
		{0x09,0x83,0x2C,0x1A,0x1B,0x6E,0x5A,0xA0,0x52,0x3B,0xD6,0xB3,0x29,0xE3,0x2F,0x84},
		{0x53,0xD1,0x00,0xED,0x20,0xFC,0xB1,0x5B,0x6A,0xCB,0xBE,0x39,0x4A,0x4C,0x58,0xCF},
		{0xD0,0xEF,0xAA,0xFB,0x43,0x4D,0x33,0x85,0x45,0xF9,0x02,0x7F,0x50,0x3C,0x9F,0xA8},
		{0x51,0xA3,0x40,0x8F,0x92,0x9D,0x38,0xF5,0xBC,0xB6,0xDA,0x21,0x10,0xFF,0xF3,0xD2},
		{0xCD,0x0C,0x13,0xEC,0x5F,0x97,0x44,0x17,0xC4,0xA7,0x7E,0x3D,0x64,0x5D,0x19,0x73},
		{0x60,0x81,0x4F,0xDC,0x22,0x2A,0x90,0x88,0x46,0xEE,0xB8,0x14,0xDE,0x5E,0x0B,0xDB},
		{0xE0,0x32,0x3A,0x0A,0x49,0x06,0x24,0x5C,0xC2,0xD3,0xAC,0x62,0x91,0x95,0xE4,0x79},
		{0xE7,0xC8,0x37,0x6D,0x8D,0xD5,0x4E,0xA9,0x6C,0x56,0xF4,0xEA,0x65,0x7A,0xAE,0x08},
		{0xBA,0x78,0x25,0x2E,0x1C,0xA6,0xB4,0xC6,0xE8,0xDD,0x74,0x1F,0x4B,0xBD,0x8B,0x8A},
		{0x70,0x3E,0xB5,0x66,0x48,0x03,0xF6,0x0E,0x61,0x35,0x57,0xB9,0x86,0xC1,0x1D,0x9E},
		{0xE1,0xF8,0x98,0x11,0x69,0xD9,0x8E,0x94,0x9B,0x1E,0x87,0xE9,0xCE,0x55,0x28,0xDF},
		{0x8C,0xA1,0x89,0x0D,0xBF,0xE6,0x42,0x68,0x41,0x99,0x2D,0x0F,0xB0,0x54,0xBB,0x16}
	};
	const unsigned char gf_mul[256][6] = 
	{
		{0x00,0x00,0x00,0x00,0x00,0x00},{0x02,0x03,0x09,0x0b,0x0d,0x0e},
		{0x04,0x06,0x12,0x16,0x1a,0x1c},{0x06,0x05,0x1b,0x1d,0x17,0x12},
		{0x08,0x0c,0x24,0x2c,0x34,0x38},{0x0a,0x0f,0x2d,0x27,0x39,0x36},
		{0x0c,0x0a,0x36,0x3a,0x2e,0x24},{0x0e,0x09,0x3f,0x31,0x23,0x2a},
		{0x10,0x18,0x48,0x58,0x68,0x70},{0x12,0x1b,0x41,0x53,0x65,0x7e},
		{0x14,0x1e,0x5a,0x4e,0x72,0x6c},{0x16,0x1d,0x53,0x45,0x7f,0x62},
		{0x18,0x14,0x6c,0x74,0x5c,0x48},{0x1a,0x17,0x65,0x7f,0x51,0x46},
		{0x1c,0x12,0x7e,0x62,0x46,0x54},{0x1e,0x11,0x77,0x69,0x4b,0x5a},
		{0x20,0x30,0x90,0xb0,0xd0,0xe0},{0x22,0x33,0x99,0xbb,0xdd,0xee},
		{0x24,0x36,0x82,0xa6,0xca,0xfc},{0x26,0x35,0x8b,0xad,0xc7,0xf2},
		{0x28,0x3c,0xb4,0x9c,0xe4,0xd8},{0x2a,0x3f,0xbd,0x97,0xe9,0xd6},
		{0x2c,0x3a,0xa6,0x8a,0xfe,0xc4},{0x2e,0x39,0xaf,0x81,0xf3,0xca},
		{0x30,0x28,0xd8,0xe8,0xb8,0x90},{0x32,0x2b,0xd1,0xe3,0xb5,0x9e},
		{0x34,0x2e,0xca,0xfe,0xa2,0x8c},{0x36,0x2d,0xc3,0xf5,0xaf,0x82},
		{0x38,0x24,0xfc,0xc4,0x8c,0xa8},{0x3a,0x27,0xf5,0xcf,0x81,0xa6},
		{0x3c,0x22,0xee,0xd2,0x96,0xb4},{0x3e,0x21,0xe7,0xd9,0x9b,0xba},
		{0x40,0x60,0x3b,0x7b,0xbb,0xdb},{0x42,0x63,0x32,0x70,0xb6,0xd5},
		{0x44,0x66,0x29,0x6d,0xa1,0xc7},{0x46,0x65,0x20,0x66,0xac,0xc9},
		{0x48,0x6c,0x1f,0x57,0x8f,0xe3},{0x4a,0x6f,0x16,0x5c,0x82,0xed},
		{0x4c,0x6a,0x0d,0x41,0x95,0xff},{0x4e,0x69,0x04,0x4a,0x98,0xf1},
		{0x50,0x78,0x73,0x23,0xd3,0xab},{0x52,0x7b,0x7a,0x28,0xde,0xa5},
		{0x54,0x7e,0x61,0x35,0xc9,0xb7},{0x56,0x7d,0x68,0x3e,0xc4,0xb9},
		{0x58,0x74,0x57,0x0f,0xe7,0x93},{0x5a,0x77,0x5e,0x04,0xea,0x9d},
		{0x5c,0x72,0x45,0x19,0xfd,0x8f},{0x5e,0x71,0x4c,0x12,0xf0,0x81},
		{0x60,0x50,0xab,0xcb,0x6b,0x3b},{0x62,0x53,0xa2,0xc0,0x66,0x35},
		{0x64,0x56,0xb9,0xdd,0x71,0x27},{0x66,0x55,0xb0,0xd6,0x7c,0x29},
		{0x68,0x5c,0x8f,0xe7,0x5f,0x03},{0x6a,0x5f,0x86,0xec,0x52,0x0d},
		{0x6c,0x5a,0x9d,0xf1,0x45,0x1f},{0x6e,0x59,0x94,0xfa,0x48,0x11},
		{0x70,0x48,0xe3,0x93,0x03,0x4b},{0x72,0x4b,0xea,0x98,0x0e,0x45},
		{0x74,0x4e,0xf1,0x85,0x19,0x57},{0x76,0x4d,0xf8,0x8e,0x14,0x59},
		{0x78,0x44,0xc7,0xbf,0x37,0x73},{0x7a,0x47,0xce,0xb4,0x3a,0x7d},
		{0x7c,0x42,0xd5,0xa9,0x2d,0x6f},{0x7e,0x41,0xdc,0xa2,0x20,0x61},
		{0x80,0xc0,0x76,0xf6,0x6d,0xad},{0x82,0xc3,0x7f,0xfd,0x60,0xa3},
		{0x84,0xc6,0x64,0xe0,0x77,0xb1},{0x86,0xc5,0x6d,0xeb,0x7a,0xbf},
		{0x88,0xcc,0x52,0xda,0x59,0x95},{0x8a,0xcf,0x5b,0xd1,0x54,0x9b},
		{0x8c,0xca,0x40,0xcc,0x43,0x89},{0x8e,0xc9,0x49,0xc7,0x4e,0x87},
		{0x90,0xd8,0x3e,0xae,0x05,0xdd},{0x92,0xdb,0x37,0xa5,0x08,0xd3},
		{0x94,0xde,0x2c,0xb8,0x1f,0xc1},{0x96,0xdd,0x25,0xb3,0x12,0xcf},
		{0x98,0xd4,0x1a,0x82,0x31,0xe5},{0x9a,0xd7,0x13,0x89,0x3c,0xeb},
		{0x9c,0xd2,0x08,0x94,0x2b,0xf9},{0x9e,0xd1,0x01,0x9f,0x26,0xf7},
		{0xa0,0xf0,0xe6,0x46,0xbd,0x4d},{0xa2,0xf3,0xef,0x4d,0xb0,0x43},
		{0xa4,0xf6,0xf4,0x50,0xa7,0x51},{0xa6,0xf5,0xfd,0x5b,0xaa,0x5f},
		{0xa8,0xfc,0xc2,0x6a,0x89,0x75},{0xaa,0xff,0xcb,0x61,0x84,0x7b},
		{0xac,0xfa,0xd0,0x7c,0x93,0x69},{0xae,0xf9,0xd9,0x77,0x9e,0x67},
		{0xb0,0xe8,0xae,0x1e,0xd5,0x3d},{0xb2,0xeb,0xa7,0x15,0xd8,0x33},
		{0xb4,0xee,0xbc,0x08,0xcf,0x21},{0xb6,0xed,0xb5,0x03,0xc2,0x2f},
		{0xb8,0xe4,0x8a,0x32,0xe1,0x05},{0xba,0xe7,0x83,0x39,0xec,0x0b},
		{0xbc,0xe2,0x98,0x24,0xfb,0x19},{0xbe,0xe1,0x91,0x2f,0xf6,0x17},
		{0xc0,0xa0,0x4d,0x8d,0xd6,0x76},{0xc2,0xa3,0x44,0x86,0xdb,0x78},
		{0xc4,0xa6,0x5f,0x9b,0xcc,0x6a},{0xc6,0xa5,0x56,0x90,0xc1,0x64},
		{0xc8,0xac,0x69,0xa1,0xe2,0x4e},{0xca,0xaf,0x60,0xaa,0xef,0x40},
		{0xcc,0xaa,0x7b,0xb7,0xf8,0x52},{0xce,0xa9,0x72,0xbc,0xf5,0x5c},
		{0xd0,0xb8,0x05,0xd5,0xbe,0x06},{0xd2,0xbb,0x0c,0xde,0xb3,0x08},
		{0xd4,0xbe,0x17,0xc3,0xa4,0x1a},{0xd6,0xbd,0x1e,0xc8,0xa9,0x14},
		{0xd8,0xb4,0x21,0xf9,0x8a,0x3e},{0xda,0xb7,0x28,0xf2,0x87,0x30},
		{0xdc,0xb2,0x33,0xef,0x90,0x22},{0xde,0xb1,0x3a,0xe4,0x9d,0x2c},
		{0xe0,0x90,0xdd,0x3d,0x06,0x96},{0xe2,0x93,0xd4,0x36,0x0b,0x98},
		{0xe4,0x96,0xcf,0x2b,0x1c,0x8a},{0xe6,0x95,0xc6,0x20,0x11,0x84},
		{0xe8,0x9c,0xf9,0x11,0x32,0xae},{0xea,0x9f,0xf0,0x1a,0x3f,0xa0},
		{0xec,0x9a,0xeb,0x07,0x28,0xb2},{0xee,0x99,0xe2,0x0c,0x25,0xbc},
		{0xf0,0x88,0x95,0x65,0x6e,0xe6},{0xf2,0x8b,0x9c,0x6e,0x63,0xe8},
		{0xf4,0x8e,0x87,0x73,0x74,0xfa},{0xf6,0x8d,0x8e,0x78,0x79,0xf4},
		{0xf8,0x84,0xb1,0x49,0x5a,0xde},{0xfa,0x87,0xb8,0x42,0x57,0xd0},
		{0xfc,0x82,0xa3,0x5f,0x40,0xc2},{0xfe,0x81,0xaa,0x54,0x4d,0xcc},
		{0x1b,0x9b,0xec,0xf7,0xda,0x41},{0x19,0x98,0xe5,0xfc,0xd7,0x4f},
		{0x1f,0x9d,0xfe,0xe1,0xc0,0x5d},{0x1d,0x9e,0xf7,0xea,0xcd,0x53},
		{0x13,0x97,0xc8,0xdb,0xee,0x79},{0x11,0x94,0xc1,0xd0,0xe3,0x77},
		{0x17,0x91,0xda,0xcd,0xf4,0x65},{0x15,0x92,0xd3,0xc6,0xf9,0x6b},
		{0x0b,0x83,0xa4,0xaf,0xb2,0x31},{0x09,0x80,0xad,0xa4,0xbf,0x3f},
		{0x0f,0x85,0xb6,0xb9,0xa8,0x2d},{0x0d,0x86,0xbf,0xb2,0xa5,0x23},
		{0x03,0x8f,0x80,0x83,0x86,0x09},{0x01,0x8c,0x89,0x88,0x8b,0x07},
		{0x07,0x89,0x92,0x95,0x9c,0x15},{0x05,0x8a,0x9b,0x9e,0x91,0x1b},
		{0x3b,0xab,0x7c,0x47,0x0a,0xa1},{0x39,0xa8,0x75,0x4c,0x07,0xaf},
		{0x3f,0xad,0x6e,0x51,0x10,0xbd},{0x3d,0xae,0x67,0x5a,0x1d,0xb3},
		{0x33,0xa7,0x58,0x6b,0x3e,0x99},{0x31,0xa4,0x51,0x60,0x33,0x97},
		{0x37,0xa1,0x4a,0x7d,0x24,0x85},{0x35,0xa2,0x43,0x76,0x29,0x8b},
		{0x2b,0xb3,0x34,0x1f,0x62,0xd1},{0x29,0xb0,0x3d,0x14,0x6f,0xdf},
		{0x2f,0xb5,0x26,0x09,0x78,0xcd},{0x2d,0xb6,0x2f,0x02,0x75,0xc3},
		{0x23,0xbf,0x10,0x33,0x56,0xe9},{0x21,0xbc,0x19,0x38,0x5b,0xe7},
		{0x27,0xb9,0x02,0x25,0x4c,0xf5},{0x25,0xba,0x0b,0x2e,0x41,0xfb},
		{0x5b,0xfb,0xd7,0x8c,0x61,0x9a},{0x59,0xf8,0xde,0x87,0x6c,0x94},
		{0x5f,0xfd,0xc5,0x9a,0x7b,0x86},{0x5d,0xfe,0xcc,0x91,0x76,0x88},
		{0x53,0xf7,0xf3,0xa0,0x55,0xa2},{0x51,0xf4,0xfa,0xab,0x58,0xac},
		{0x57,0xf1,0xe1,0xb6,0x4f,0xbe},{0x55,0xf2,0xe8,0xbd,0x42,0xb0},
		{0x4b,0xe3,0x9f,0xd4,0x09,0xea},{0x49,0xe0,0x96,0xdf,0x04,0xe4},
		{0x4f,0xe5,0x8d,0xc2,0x13,0xf6},{0x4d,0xe6,0x84,0xc9,0x1e,0xf8},
		{0x43,0xef,0xbb,0xf8,0x3d,0xd2},{0x41,0xec,0xb2,0xf3,0x30,0xdc},
		{0x47,0xe9,0xa9,0xee,0x27,0xce},{0x45,0xea,0xa0,0xe5,0x2a,0xc0},
		{0x7b,0xcb,0x47,0x3c,0xb1,0x7a},{0x79,0xc8,0x4e,0x37,0xbc,0x74},
		{0x7f,0xcd,0x55,0x2a,0xab,0x66},{0x7d,0xce,0x5c,0x21,0xa6,0x68},
		{0x73,0xc7,0x63,0x10,0x85,0x42},{0x71,0xc4,0x6a,0x1b,0x88,0x4c},
		{0x77,0xc1,0x71,0x06,0x9f,0x5e},{0x75,0xc2,0x78,0x0d,0x92,0x50},
		{0x6b,0xd3,0x0f,0x64,0xd9,0x0a},{0x69,0xd0,0x06,0x6f,0xd4,0x04},
		{0x6f,0xd5,0x1d,0x72,0xc3,0x16},{0x6d,0xd6,0x14,0x79,0xce,0x18},
		{0x63,0xdf,0x2b,0x48,0xed,0x32},{0x61,0xdc,0x22,0x43,0xe0,0x3c},
		{0x67,0xd9,0x39,0x5e,0xf7,0x2e},{0x65,0xda,0x30,0x55,0xfa,0x20},
		{0x9b,0x5b,0x9a,0x01,0xb7,0xec},{0x99,0x58,0x93,0x0a,0xba,0xe2},
		{0x9f,0x5d,0x88,0x17,0xad,0xf0},{0x9d,0x5e,0x81,0x1c,0xa0,0xfe},
		{0x93,0x57,0xbe,0x2d,0x83,0xd4},{0x91,0x54,0xb7,0x26,0x8e,0xda},
		{0x97,0x51,0xac,0x3b,0x99,0xc8},{0x95,0x52,0xa5,0x30,0x94,0xc6},
		{0x8b,0x43,0xd2,0x59,0xdf,0x9c},{0x89,0x40,0xdb,0x52,0xd2,0x92},
		{0x8f,0x45,0xc0,0x4f,0xc5,0x80},{0x8d,0x46,0xc9,0x44,0xc8,0x8e},
		{0x83,0x4f,0xf6,0x75,0xeb,0xa4},{0x81,0x4c,0xff,0x7e,0xe6,0xaa},
		{0x87,0x49,0xe4,0x63,0xf1,0xb8},{0x85,0x4a,0xed,0x68,0xfc,0xb6},
		{0xbb,0x6b,0x0a,0xb1,0x67,0x0c},{0xb9,0x68,0x03,0xba,0x6a,0x02},
		{0xbf,0x6d,0x18,0xa7,0x7d,0x10},{0xbd,0x6e,0x11,0xac,0x70,0x1e},
		{0xb3,0x67,0x2e,0x9d,0x53,0x34},{0xb1,0x64,0x27,0x96,0x5e,0x3a},
		{0xb7,0x61,0x3c,0x8b,0x49,0x28},{0xb5,0x62,0x35,0x80,0x44,0x26},
		{0xab,0x73,0x42,0xe9,0x0f,0x7c},{0xa9,0x70,0x4b,0xe2,0x02,0x72},
		{0xaf,0x75,0x50,0xff,0x15,0x60},{0xad,0x76,0x59,0xf4,0x18,0x6e},
		{0xa3,0x7f,0x66,0xc5,0x3b,0x44},{0xa1,0x7c,0x6f,0xce,0x36,0x4a},
		{0xa7,0x79,0x74,0xd3,0x21,0x58},{0xa5,0x7a,0x7d,0xd8,0x2c,0x56},
		{0xdb,0x3b,0xa1,0x7a,0x0c,0x37},{0xd9,0x38,0xa8,0x71,0x01,0x39},
		{0xdf,0x3d,0xb3,0x6c,0x16,0x2b},{0xdd,0x3e,0xba,0x67,0x1b,0x25},
		{0xd3,0x37,0x85,0x56,0x38,0x0f},{0xd1,0x34,0x8c,0x5d,0x35,0x01},
		{0xd7,0x31,0x97,0x40,0x22,0x13},{0xd5,0x32,0x9e,0x4b,0x2f,0x1d},
		{0xcb,0x23,0xe9,0x22,0x64,0x47},{0xc9,0x20,0xe0,0x29,0x69,0x49},
		{0xcf,0x25,0xfb,0x34,0x7e,0x5b},{0xcd,0x26,0xf2,0x3f,0x73,0x55},
		{0xc3,0x2f,0xcd,0x0e,0x50,0x7f},{0xc1,0x2c,0xc4,0x05,0x5d,0x71},
		{0xc7,0x29,0xdf,0x18,0x4a,0x63},{0xc5,0x2a,0xd6,0x13,0x47,0x6d},
		{0xfb,0x0b,0x31,0xca,0xdc,0xd7},{0xf9,0x08,0x38,0xc1,0xd1,0xd9},
		{0xff,0x0d,0x23,0xdc,0xc6,0xcb},{0xfd,0x0e,0x2a,0xd7,0xcb,0xc5},
		{0xf3,0x07,0x15,0xe6,0xe8,0xef},{0xf1,0x04,0x1c,0xed,0xe5,0xe1},
		{0xf7,0x01,0x07,0xf0,0xf2,0xf3},{0xf5,0x02,0x0e,0xfb,0xff,0xfd},
		{0xeb,0x13,0x79,0x92,0xb4,0xa7},{0xe9,0x10,0x70,0x99,0xb9,0xa9},
		{0xef,0x15,0x6b,0x84,0xae,0xbb},{0xed,0x16,0x62,0x8f,0xa3,0xb5},
		{0xe3,0x1f,0x5d,0xbe,0x80,0x9f},{0xe1,0x1c,0x54,0xb5,0x8d,0x91},
		{0xe7,0x19,0x4f,0xa8,0x9a,0x83},{0xe5,0x1a,0x46,0xa3,0x97,0x8d}
	};

	const unsigned char aes_invsbox[16][16] = {
		{0x52,0x09,0x6A,0xD5,0x30,0x36,0xA5,0x38,0xBF,0x40,0xA3,0x9E,0x81,0xF3,0xD7,0xFB},
		{0x7C,0xE3,0x39,0x82,0x9B,0x2F,0xFF,0x87,0x34,0x8E,0x43,0x44,0xC4,0xDE,0xE9,0xCB},
		{0x54,0x7B,0x94,0x32,0xA6,0xC2,0x23,0x3D,0xEE,0x4C,0x95,0x0B,0x42,0xFA,0xC3,0x4E},
		{0x08,0x2E,0xA1,0x66,0x28,0xD9,0x24,0xB2,0x76,0x5B,0xA2,0x49,0x6D,0x8B,0xD1,0x25},
		{0x72,0xF8,0xF6,0x64,0x86,0x68,0x98,0x16,0xD4,0xA4,0x5C,0xCC,0x5D,0x65,0xB6,0x92},
		{0x6C,0x70,0x48,0x50,0xFD,0xED,0xB9,0xDA,0x5E,0x15,0x46,0x57,0xA7,0x8D,0x9D,0x84},
		{0x90,0xD8,0xAB,0x00,0x8C,0xBC,0xD3,0x0A,0xF7,0xE4,0x58,0x05,0xB8,0xB3,0x45,0x06},
		{0xD0,0x2C,0x1E,0x8F,0xCA,0x3F,0x0F,0x02,0xC1,0xAF,0xBD,0x03,0x01,0x13,0x8A,0x6B},
		{0x3A,0x91,0x11,0x41,0x4F,0x67,0xDC,0xEA,0x97,0xF2,0xCF,0xCE,0xF0,0xB4,0xE6,0x73},
		{0x96,0xAC,0x74,0x22,0xE7,0xAD,0x35,0x85,0xE2,0xF9,0x37,0xE8,0x1C,0x75,0xDF,0x6E},
		{0x47,0xF1,0x1A,0x71,0x1D,0x29,0xC5,0x89,0x6F,0xB7,0x62,0x0E,0xAA,0x18,0xBE,0x1B},
		{0xFC,0x56,0x3E,0x4B,0xC6,0xD2,0x79,0x20,0x9A,0xDB,0xC0,0xFE,0x78,0xCD,0x5A,0xF4},
		{0x1F,0xDD,0xA8,0x33,0x88,0x07,0xC7,0x31,0xB1,0x12,0x10,0x59,0x27,0x80,0xEC,0x5F},
		{0x60,0x51,0x7F,0xA9,0x19,0xB5,0x4A,0x0D,0x2D,0xE5,0x7A,0x9F,0x93,0xC9,0x9C,0xEF},
		{0xA0,0xE0,0x3B,0x4D,0xAE,0x2A,0xF5,0xB0,0xC8,0xEB,0xBB,0x3C,0x83,0x53,0x99,0x61},
		{0x17,0x2B,0x04,0x7E,0xBA,0x77,0xD6,0x26,0xE1,0x69,0x14,0x63,0x55,0x21,0x0C,0x7D}
	};
#pragma endregion
	class cb_aes
	{
	public:
		cb_aes(unsigned int ikeysize = 128):m_ikeysize(ikeysize)
		{
			unsigned char key[32] = 
			{
				0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,
				0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,
				0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,
				0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4
			};
			aes_key_setup(key, m_key_schedule, m_ikeysize);
		}
		virtual ~cb_aes(){}
	public:
		inline unsigned int getciphersize(unsigned int isrclen)
		{
			return (((isrclen + 16 - 1) & ~(16 - 1)) + 16);
		}
		int encrypt(const char* psrc, unsigned int isrclen, char* pdes, unsigned int& ideslen)
		{
			if(!psrc || isrclen <= 0 || !pdes)
				return -1;
			unsigned int ilen = getciphersize(isrclen);
			if(ideslen < ilen)
				return -2;

			if(psrc != pdes)
				memmove(pdes, psrc, isrclen);

			*((unsigned int*)&pdes[((isrclen + 16 - 1) & ~(16 - 1))]) = isrclen;
			
			for(unsigned int i = 0; i < ilen; i += 16)
				aes_encrypt((unsigned char*)(&pdes[i]), (unsigned char*)(&pdes[i]), m_key_schedule, m_ikeysize);

			ideslen = ilen;

			return 0;
		}
		int decrypt(char* psrc, unsigned int& isrclen)
		{
			if(!psrc || isrclen <= 0){
				return -1;
			}
			if(isrclen % 16 != 0){
				return -2;
			}
			for(unsigned int i = 0; i < isrclen; i += 16)
				aes_decrypt((unsigned char*)(&psrc[i]), (unsigned char*)(&psrc[i]), m_key_schedule, m_ikeysize);
			
			isrclen = *(unsigned int*)(&psrc[isrclen - 16]);

			return 0;
		}
	private:
		void aes_encrypt(const unsigned char in[], unsigned char out[], const unsigned int key[], int keysize)
		{
			unsigned char state[4][4];
			// Copy input array (should be 16 bytes long) to a matrix (sequential bytes are ordered
			// by row, not col) called "state" for processing.
			// *** Implementation note: The official AES documentation references the state by
			// column, then row. Accessing an element in C requires row then column. Thus, all state
			// references in AES must have the column and row indexes reversed for C implementation.
			state[0][0] = in[0];
			state[1][0] = in[1];
			state[2][0] = in[2];
			state[3][0] = in[3];
			state[0][1] = in[4];
			state[1][1] = in[5];
			state[2][1] = in[6];
			state[3][1] = in[7];
			state[0][2] = in[8];
			state[1][2] = in[9];
			state[2][2] = in[10];
			state[3][2] = in[11];
			state[0][3] = in[12];
			state[1][3] = in[13];
			state[2][3] = in[14];
			state[3][3] = in[15];
			// Perform the necessary number of rounds. The round key is added first.
			// The last round does not perform the mixcolumns step.
			addroundkey(state,&key[0]);
			subbytes(state); shiftrows(state); mixcolumns(state); addroundkey(state,&key[4]);
			subbytes(state); shiftrows(state); mixcolumns(state); addroundkey(state,&key[8]);
			subbytes(state); shiftrows(state); mixcolumns(state); addroundkey(state,&key[12]);
			subbytes(state); shiftrows(state); mixcolumns(state); addroundkey(state,&key[16]);
			subbytes(state); shiftrows(state); mixcolumns(state); addroundkey(state,&key[20]);
			subbytes(state); shiftrows(state); mixcolumns(state); addroundkey(state,&key[24]);
			subbytes(state); shiftrows(state); mixcolumns(state); addroundkey(state,&key[28]);
			subbytes(state); shiftrows(state); mixcolumns(state); addroundkey(state,&key[32]);
			subbytes(state); shiftrows(state); mixcolumns(state); addroundkey(state,&key[36]);
			if(keysize != 128)
			{
				subbytes(state); shiftrows(state); mixcolumns(state); addroundkey(state,&key[40]);
				subbytes(state); shiftrows(state); mixcolumns(state); addroundkey(state,&key[44]);
				if(keysize != 192)
				{
					subbytes(state); shiftrows(state); mixcolumns(state); addroundkey(state,&key[48]);
					subbytes(state); shiftrows(state); mixcolumns(state); addroundkey(state,&key[52]);
					subbytes(state); shiftrows(state); addroundkey(state,&key[56]);
				}
				else{
					subbytes(state); shiftrows(state); addroundkey(state,&key[48]);
				}
			}
			else{
				subbytes(state); shiftrows(state); addroundkey(state,&key[40]);
			}
			// Copy the state to the output array.
			out[0]  = state[0][0];
			out[1]  = state[1][0];
			out[2]  = state[2][0];
			out[3]  = state[3][0];
			out[4]  = state[0][1];
			out[5]  = state[1][1];
			out[6]  = state[2][1];
			out[7]  = state[3][1];
			out[8]  = state[0][2];
			out[9]  = state[1][2];
			out[10] = state[2][2];
			out[11] = state[3][2];
			out[12] = state[0][3];
			out[13] = state[1][3];
			out[14] = state[2][3];
			out[15] = state[3][3];
		}
		void aes_decrypt(const unsigned char in[], unsigned char out[], const unsigned int key[], int keysize)
		{
			unsigned char state[4][4];
			// copy the input to the state.
			state[0][0] = in[0];
			state[1][0] = in[1];
			state[2][0] = in[2];
			state[3][0] = in[3];
			state[0][1] = in[4];
			state[1][1] = in[5];
			state[2][1] = in[6];
			state[3][1] = in[7];
			state[0][2] = in[8];
			state[1][2] = in[9];
			state[2][2] = in[10];
			state[3][2] = in[11];
			state[0][3] = in[12];
			state[1][3] = in[13];
			state[2][3] = in[14];
			state[3][3] = in[15];
			// Perform the necessary number of rounds. The round key is added first.
			// The last round does not perform the MixColumns step.
			if(keysize > 128)
			{
				if(keysize > 192)
				{
					addroundkey(state, &key[56]);
					invshiftrows(state);invsubbytes(state);addroundkey(state, &key[52]);invmixcolumns(state);
					invshiftrows(state);invsubbytes(state);addroundkey(state, &key[48]);invmixcolumns(state);
				}
				else{
					addroundkey(state, &key[48]);
				}
				invshiftrows(state);invsubbytes(state);addroundkey(state, &key[44]);invmixcolumns(state);
				invshiftrows(state);invsubbytes(state);addroundkey(state, &key[40]);invmixcolumns(state);
			}
			else{
				addroundkey(state, &key[40]);
			}
			invshiftrows(state);invsubbytes(state);addroundkey(state, &key[36]);invmixcolumns(state);
			invshiftrows(state);invsubbytes(state);addroundkey(state, &key[32]);invmixcolumns(state);
			invshiftrows(state);invsubbytes(state);addroundkey(state, &key[28]);invmixcolumns(state);
			invshiftrows(state);invsubbytes(state);addroundkey(state, &key[24]);invmixcolumns(state);
			invshiftrows(state);invsubbytes(state);addroundkey(state, &key[20]);invmixcolumns(state);
			invshiftrows(state);invsubbytes(state);addroundkey(state, &key[16]);invmixcolumns(state);
			invshiftrows(state);invsubbytes(state);addroundkey(state, &key[12]);invmixcolumns(state);
			invshiftrows(state);invsubbytes(state);addroundkey(state, &key[8] );invmixcolumns(state);
			invshiftrows(state);invsubbytes(state);addroundkey(state, &key[4] );invmixcolumns(state);
			invshiftrows(state);invsubbytes(state);addroundkey(state, &key[0] );
			// copy the state to the output array.
			out[0] = state[0][0];
			out[1] = state[1][0];
			out[2] = state[2][0];
			out[3] = state[3][0];
			out[4] = state[0][1];
			out[5] = state[1][1];
			out[6] = state[2][1];
			out[7] = state[3][1];
			out[8] = state[0][2];
			out[9] = state[1][2];
			out[10] = state[2][2];
			out[11] = state[3][2];
			out[12] = state[0][3];
			out[13] = state[1][3];
			out[14] = state[2][3];
			out[15] = state[3][3];
		}

		void addroundkey(unsigned char state[][4], const unsigned int w[])
		{
			unsigned char subkey[4];
			// memcpy(subkey, &w[idx], 4); // Not accurate for big endian machines
			// subkey 1
			subkey[0] = w[0] >> 24;
			subkey[1] = w[0] >> 16;
			subkey[2] = w[0] >>  8;
			subkey[3] = w[0];
			state[0][0] ^= subkey[0];
			state[1][0] ^= subkey[1];
			state[2][0] ^= subkey[2];
			state[3][0] ^= subkey[3];
			// subkey 2
			subkey[0] = w[1] >> 24;
			subkey[1] = w[1] >> 16;
			subkey[2] = w[1] >>  8;
			subkey[3] = w[1];
			state[0][1] ^= subkey[0];
			state[1][1] ^= subkey[1];
			state[2][1] ^= subkey[2];
			state[3][1] ^= subkey[3];
			// subkey 3
			subkey[0] = w[2] >> 24;
			subkey[1] = w[2] >> 16;
			subkey[2] = w[2] >>  8;
			subkey[3] = w[2];
			state[0][2] ^= subkey[0];
			state[1][2] ^= subkey[1];
			state[2][2] ^= subkey[2];
			state[3][2] ^= subkey[3];
			// subkey 4
			subkey[0] = w[3] >> 24;
			subkey[1] = w[3] >> 16;
			subkey[2] = w[3] >>  8;
			subkey[3] = w[3];
			state[0][3] ^= subkey[0];
			state[1][3] ^= subkey[1];
			state[2][3] ^= subkey[2];
			state[3][3] ^= subkey[3];
		}
		void subbytes(unsigned char state[][4])
		{
			
			state[0][0] = aes_sbox[state[0][0] >> 4][state[0][0] & 0x0F];
			state[0][1] = aes_sbox[state[0][1] >> 4][state[0][1] & 0x0F];
			state[0][2] = aes_sbox[state[0][2] >> 4][state[0][2] & 0x0F];
			state[0][3] = aes_sbox[state[0][3] >> 4][state[0][3] & 0x0F];
			state[1][0] = aes_sbox[state[1][0] >> 4][state[1][0] & 0x0F];
			state[1][1] = aes_sbox[state[1][1] >> 4][state[1][1] & 0x0F];
			state[1][2] = aes_sbox[state[1][2] >> 4][state[1][2] & 0x0F];
			state[1][3] = aes_sbox[state[1][3] >> 4][state[1][3] & 0x0F];
			state[2][0] = aes_sbox[state[2][0] >> 4][state[2][0] & 0x0F];
			state[2][1] = aes_sbox[state[2][1] >> 4][state[2][1] & 0x0F];
			state[2][2] = aes_sbox[state[2][2] >> 4][state[2][2] & 0x0F];
			state[2][3] = aes_sbox[state[2][3] >> 4][state[2][3] & 0x0F];
			state[3][0] = aes_sbox[state[3][0] >> 4][state[3][0] & 0x0F];
			state[3][1] = aes_sbox[state[3][1] >> 4][state[3][1] & 0x0F];
			state[3][2] = aes_sbox[state[3][2] >> 4][state[3][2] & 0x0F];
			state[3][3] = aes_sbox[state[3][3] >> 4][state[3][3] & 0x0F];
		}
		void shiftrows(unsigned char state[][4])
		{
			// shift left by 1
			int t = state[1][0];
			state[1][0] = state[1][1];
			state[1][1] = state[1][2];
			state[1][2] = state[1][3];
			state[1][3] = t;
			// shift left by 2
			t = state[2][0];
			state[2][0] = state[2][2];
			state[2][2] = t;
			t = state[2][1];
			state[2][1] = state[2][3];
			state[2][3] = t;
			// shift left by 3
			t = state[3][0];
			state[3][0] = state[3][3];
			state[3][3] = state[3][2];
			state[3][2] = state[3][1];
			state[3][1] = t;
		}
		void mixcolumns(unsigned char state[][4])
		{
			unsigned char col[4];
			// column 1
			col[0] = state[0][0];
			col[1] = state[1][0];
			col[2] = state[2][0];
			col[3] = state[3][0];
			state[0][0]  = gf_mul[col[0]][0];
			state[0][0] ^= gf_mul[col[1]][1];
			state[0][0] ^= col[2];
			state[0][0] ^= col[3];
			state[1][0]  = col[0];
			state[1][0] ^= gf_mul[col[1]][0];
			state[1][0] ^= gf_mul[col[2]][1];
			state[1][0] ^= col[3];
			state[2][0]  = col[0];
			state[2][0] ^= col[1];
			state[2][0] ^= gf_mul[col[2]][0];
			state[2][0] ^= gf_mul[col[3]][1];
			state[3][0]  = gf_mul[col[0]][1];
			state[3][0] ^= col[1];
			state[3][0] ^= col[2];
			state[3][0] ^= gf_mul[col[3]][0];
			// column 2
			col[0] = state[0][1];
			col[1] = state[1][1];
			col[2] = state[2][1];
			col[3] = state[3][1];
			state[0][1]  = gf_mul[col[0]][0];
			state[0][1] ^= gf_mul[col[1]][1];
			state[0][1] ^= col[2];
			state[0][1] ^= col[3];
			state[1][1]  = col[0];
			state[1][1] ^= gf_mul[col[1]][0];
			state[1][1] ^= gf_mul[col[2]][1];
			state[1][1] ^= col[3];
			state[2][1]  = col[0];
			state[2][1] ^= col[1];
			state[2][1] ^= gf_mul[col[2]][0];
			state[2][1] ^= gf_mul[col[3]][1];
			state[3][1]  = gf_mul[col[0]][1];
			state[3][1] ^= col[1];
			state[3][1] ^= col[2];
			state[3][1] ^= gf_mul[col[3]][0];
			// column 3
			col[0] = state[0][2];
			col[1] = state[1][2];
			col[2] = state[2][2];
			col[3] = state[3][2];
			state[0][2]  = gf_mul[col[0]][0];
			state[0][2] ^= gf_mul[col[1]][1];
			state[0][2] ^= col[2];
			state[0][2] ^= col[3];
			state[1][2]  = col[0];
			state[1][2] ^= gf_mul[col[1]][0];
			state[1][2] ^= gf_mul[col[2]][1];
			state[1][2] ^= col[3];
			state[2][2]  = col[0];
			state[2][2] ^= col[1];
			state[2][2] ^= gf_mul[col[2]][0];
			state[2][2] ^= gf_mul[col[3]][1];
			state[3][2]  = gf_mul[col[0]][1];
			state[3][2] ^= col[1];
			state[3][2] ^= col[2];
			state[3][2] ^= gf_mul[col[3]][0];
			// column 4
			col[0] = state[0][3];
			col[1] = state[1][3];
			col[2] = state[2][3];
			col[3] = state[3][3];
			state[0][3]  = gf_mul[col[0]][0];
			state[0][3] ^= gf_mul[col[1]][1];
			state[0][3] ^= col[2];
			state[0][3] ^= col[3];
			state[1][3]  = col[0];
			state[1][3] ^= gf_mul[col[1]][0];
			state[1][3] ^= gf_mul[col[2]][1];
			state[1][3] ^= col[3];
			state[2][3]  = col[0];
			state[2][3] ^= col[1];
			state[2][3] ^= gf_mul[col[2]][0];
			state[2][3] ^= gf_mul[col[3]][1];
			state[3][3]  = gf_mul[col[0]][1];
			state[3][3] ^= col[1];
			state[3][3] ^= col[2];
			state[3][3] ^= gf_mul[col[3]][0];
		}

		void invshiftrows(unsigned char state[][4])
		{
			// shift right by 1
			int t = state[1][3];
			state[1][3] = state[1][2];
			state[1][2] = state[1][1];
			state[1][1] = state[1][0];
			state[1][0] = t;
			// shift right by 2
			t = state[2][3];
			state[2][3] = state[2][1];
			state[2][1] = t;
			t = state[2][2];
			state[2][2] = state[2][0];
			state[2][0] = t;
			// shift right by 3
			t = state[3][3];
			state[3][3] = state[3][0];
			state[3][0] = state[3][1];
			state[3][1] = state[3][2];
			state[3][2] = t;
		}
		void invsubbytes(unsigned char state[][4])
		{
			state[0][0] = aes_invsbox[state[0][0] >> 4][state[0][0] & 0x0F];
			state[0][1] = aes_invsbox[state[0][1] >> 4][state[0][1] & 0x0F];
			state[0][2] = aes_invsbox[state[0][2] >> 4][state[0][2] & 0x0F];
			state[0][3] = aes_invsbox[state[0][3] >> 4][state[0][3] & 0x0F];
			state[1][0] = aes_invsbox[state[1][0] >> 4][state[1][0] & 0x0F];
			state[1][1] = aes_invsbox[state[1][1] >> 4][state[1][1] & 0x0F];
			state[1][2] = aes_invsbox[state[1][2] >> 4][state[1][2] & 0x0F];
			state[1][3] = aes_invsbox[state[1][3] >> 4][state[1][3] & 0x0F];
			state[2][0] = aes_invsbox[state[2][0] >> 4][state[2][0] & 0x0F];
			state[2][1] = aes_invsbox[state[2][1] >> 4][state[2][1] & 0x0F];
			state[2][2] = aes_invsbox[state[2][2] >> 4][state[2][2] & 0x0F];
			state[2][3] = aes_invsbox[state[2][3] >> 4][state[2][3] & 0x0F];
			state[3][0] = aes_invsbox[state[3][0] >> 4][state[3][0] & 0x0F];
			state[3][1] = aes_invsbox[state[3][1] >> 4][state[3][1] & 0x0F];
			state[3][2] = aes_invsbox[state[3][2] >> 4][state[3][2] & 0x0F];
			state[3][3] = aes_invsbox[state[3][3] >> 4][state[3][3] & 0x0F];
		}
		void invmixcolumns(unsigned char state[][4])
		{
			unsigned char col[4];
			// column 1
			col[0] = state[0][0];
			col[1] = state[1][0];
			col[2] = state[2][0];
			col[3] = state[3][0];
			state[0][0]  = gf_mul[col[0]][5];
			state[0][0] ^= gf_mul[col[1]][3];
			state[0][0] ^= gf_mul[col[2]][4];
			state[0][0] ^= gf_mul[col[3]][2];
			state[1][0]  = gf_mul[col[0]][2];
			state[1][0] ^= gf_mul[col[1]][5];
			state[1][0] ^= gf_mul[col[2]][3];
			state[1][0] ^= gf_mul[col[3]][4];
			state[2][0]  = gf_mul[col[0]][4];
			state[2][0] ^= gf_mul[col[1]][2];
			state[2][0] ^= gf_mul[col[2]][5];
			state[2][0] ^= gf_mul[col[3]][3];
			state[3][0]  = gf_mul[col[0]][3];
			state[3][0] ^= gf_mul[col[1]][4];
			state[3][0] ^= gf_mul[col[2]][2];
			state[3][0] ^= gf_mul[col[3]][5];
			// column 2
			col[0] = state[0][1];
			col[1] = state[1][1];
			col[2] = state[2][1];
			col[3] = state[3][1];
			state[0][1]  = gf_mul[col[0]][5];
			state[0][1] ^= gf_mul[col[1]][3];
			state[0][1] ^= gf_mul[col[2]][4];
			state[0][1] ^= gf_mul[col[3]][2];
			state[1][1]  = gf_mul[col[0]][2];
			state[1][1] ^= gf_mul[col[1]][5];
			state[1][1] ^= gf_mul[col[2]][3];
			state[1][1] ^= gf_mul[col[3]][4];
			state[2][1]  = gf_mul[col[0]][4];
			state[2][1] ^= gf_mul[col[1]][2];
			state[2][1] ^= gf_mul[col[2]][5];
			state[2][1] ^= gf_mul[col[3]][3];
			state[3][1]  = gf_mul[col[0]][3];
			state[3][1] ^= gf_mul[col[1]][4];
			state[3][1] ^= gf_mul[col[2]][2];
			state[3][1] ^= gf_mul[col[3]][5];
			// column 3
			col[0] = state[0][2];
			col[1] = state[1][2];
			col[2] = state[2][2];
			col[3] = state[3][2];
			state[0][2]  = gf_mul[col[0]][5];
			state[0][2] ^= gf_mul[col[1]][3];
			state[0][2] ^= gf_mul[col[2]][4];
			state[0][2] ^= gf_mul[col[3]][2];
			state[1][2]  = gf_mul[col[0]][2];
			state[1][2] ^= gf_mul[col[1]][5];
			state[1][2] ^= gf_mul[col[2]][3];
			state[1][2] ^= gf_mul[col[3]][4];
			state[2][2]  = gf_mul[col[0]][4];
			state[2][2] ^= gf_mul[col[1]][2];
			state[2][2] ^= gf_mul[col[2]][5];
			state[2][2] ^= gf_mul[col[3]][3];
			state[3][2]  = gf_mul[col[0]][3];
			state[3][2] ^= gf_mul[col[1]][4];
			state[3][2] ^= gf_mul[col[2]][2];
			state[3][2] ^= gf_mul[col[3]][5];
			// column 4
			col[0] = state[0][3];
			col[1] = state[1][3];
			col[2] = state[2][3];
			col[3] = state[3][3];
			state[0][3]  = gf_mul[col[0]][5];
			state[0][3] ^= gf_mul[col[1]][3];
			state[0][3] ^= gf_mul[col[2]][4];
			state[0][3] ^= gf_mul[col[3]][2];
			state[1][3]  = gf_mul[col[0]][2];
			state[1][3] ^= gf_mul[col[1]][5];
			state[1][3] ^= gf_mul[col[2]][3];
			state[1][3] ^= gf_mul[col[3]][4];
			state[2][3]  = gf_mul[col[0]][4];
			state[2][3] ^= gf_mul[col[1]][2];
			state[2][3] ^= gf_mul[col[2]][5];
			state[2][3] ^= gf_mul[col[3]][3];
			state[3][3]  = gf_mul[col[0]][3];
			state[3][3] ^= gf_mul[col[1]][4];
			state[3][3] ^= gf_mul[col[2]][2];
			state[3][3] ^= gf_mul[col[3]][5];
		}

		unsigned int subword(unsigned int word)
		{
			unsigned int result = (int)aes_sbox[(word >>  4) & 0x0000000F][word & 0x0000000F];
			result += (int)aes_sbox[(word >> 12) & 0x0000000F][(word >>  8) & 0x0000000F] <<  8;
			result += (int)aes_sbox[(word >> 20) & 0x0000000F][(word >> 16) & 0x0000000F] << 16;
			result += (int)aes_sbox[(word >> 28) & 0x0000000F][(word >> 24) & 0x0000000F] << 24;
			return (result);
		}
		void aes_key_setup(const unsigned char key[], unsigned int w[], int keysize)
		{
			int Nb = 4, Nr = 0, Nk = 0, idx = 0;
			unsigned int temp, Rcon[] = 
			{
				0x01000000,0x02000000,0x04000000,0x08000000,0x10000000,0x20000000,
				0x40000000,0x80000000,0x1b000000,0x36000000,0x6c000000,0xd8000000,
				0xab000000,0x4d000000,0x9a000000
			};
			switch(keysize)
			{
			case 128:
				Nr = 10; Nk = 4; break;//16 key
			case 192:
				Nr = 12; Nk = 6; break;//24 key
			case 256:
				Nr = 14; Nk = 8; break;//32 key
			default:
				return;
			}
			for(idx = 0; idx < Nk; ++idx)
			{
				w[idx] = ((key[4 * idx]) << 24) | ((key[4 * idx + 1]) << 16) |
					((key[4 * idx + 2]) << 8) | ((key[4 * idx + 3]));
			}
			int isize = Nb * (Nr+1);
			for(idx = Nk; idx < isize; ++idx)
			{
				temp = w[idx - 1];
				if((idx % Nk) == 0)
				{
					temp = subword(((temp << 8) | (temp >> 24))) ^ Rcon[(idx-1)/Nk];
				}
				else if(Nk > 6 && (idx % Nk) == 4){
					temp = subword(temp);
				}
				w[idx] = w[idx-Nk] ^ temp;
			}
		}
	private:
		unsigned int m_ikeysize;
		unsigned int m_key_schedule[60];
	};

#pragma region blowfish
	const unsigned int p_perm[18] = 
	{
		0x243F6A88,0x85A308D3,0x13198A2E,0x03707344,0xA4093822,0x299F31D0,0x082EFA98,
		0xEC4E6C89,0x452821E6,0x38D01377,0xBE5466CF,0x34E90C6C,0xC0AC29B7,0xC97C50DD,
		0x3F84D5B5,0xB5470917,0x9216D5D9,0x8979FB1B
	};
	const unsigned int s_perm[4][256] =
	{
		{
			0xD1310BA6,0x98DFB5AC,0x2FFD72DB,0xD01ADFB7,0xB8E1AFED,0x6A267E96,0xBA7C9045,0xF12C7F99,
			0x24A19947,0xB3916CF7,0x0801F2E2,0x858EFC16,0x636920D8,0x71574E69,0xA458FEA3,0xF4933D7E,
			0x0D95748F,0x728EB658,0x718BCD58,0x82154AEE,0x7B54A41D,0xC25A59B5,0x9C30D539,0x2AF26013,
			0xC5D1B023,0x286085F0,0xCA417918,0xB8DB38EF,0x8E79DCB0,0x603A180E,0x6C9E0E8B,0xB01E8A3E,
			0xD71577C1,0xBD314B27,0x78AF2FDA,0x55605C60,0xE65525F3,0xAA55AB94,0x57489862,0x63E81440,
			0x55CA396A,0x2AAB10B6,0xB4CC5C34,0x1141E8CE,0xA15486AF,0x7C72E993,0xB3EE1411,0x636FBC2A,
			0x2BA9C55D,0x741831F6,0xCE5C3E16,0x9B87931E,0xAFD6BA33,0x6C24CF5C,0x7A325381,0x28958677,
			0x3B8F4898,0x6B4BB9AF,0xC4BFE81B,0x66282193,0x61D809CC,0xFB21A991,0x487CAC60,0x5DEC8032,
			0xEF845D5D,0xE98575B1,0xDC262302,0xEB651B88,0x23893E81,0xD396ACC5,0x0F6D6FF3,0x83F44239,
			0x2E0B4482,0xA4842004,0x69C8F04A,0x9E1F9B5E,0x21C66842,0xF6E96C9A,0x670C9C61,0xABD388F0,
			0x6A51A0D2,0xD8542F68,0x960FA728,0xAB5133A3,0x6EEF0B6C,0x137A3BE4,0xBA3BF050,0x7EFB2A98,
			0xA1F1651D,0x39AF0176,0x66CA593E,0x82430E88,0x8CEE8619,0x456F9FB4,0x7D84A5C3,0x3B8B5EBE,
			0xE06F75D8,0x85C12073,0x401A449F,0x56C16AA6,0x4ED3AA62,0x363F7706,0x1BFEDF72,0x429B023D,
			0x37D0D724,0xD00A1248,0xDB0FEAD3,0x49F1C09B,0x075372C9,0x80991B7B,0x25D479D8,0xF6E8DEF7,
			0xE3FE501A,0xB6794C3B,0x976CE0BD,0x04C006BA,0xC1A94FB6,0x409F60C4,0x5E5C9EC2,0x196A2463,
			0x68FB6FAF,0x3E6C53B5,0x1339B2EB,0x3B52EC6F,0x6DFC511F,0x9B30952C,0xCC814544,0xAF5EBD09,
			0xBEE3D004,0xDE334AFD,0x660F2807,0x192E4BB3,0xC0CBA857,0x45C8740F,0xD20B5F39,0xB9D3FBDB,
			0x5579C0BD,0x1A60320A,0xD6A100C6,0x402C7279,0x679F25FE,0xFB1FA3CC,0x8EA5E9F8,0xDB3222F8,
			0x3C7516DF,0xFD616B15,0x2F501EC8,0xAD0552AB,0x323DB5FA,0xFD238760,0x53317B48,0x3E00DF82,
			0x9E5C57BB,0xCA6F8CA0,0x1A87562E,0xDF1769DB,0xD542A8F6,0x287EFFC3,0xAC6732C6,0x8C4F5573,
			0x695B27B0,0xBBCA58C8,0xE1FFA35D,0xB8F011A0,0x10FA3D98,0xFD2183B8,0x4AFCB56C,0x2DD1D35B,
			0x9A53E479,0xB6F84565,0xD28E49BC,0x4BFB9790,0xE1DDF2DA,0xA4CB7E33,0x62FB1341,0xCEE4C6E8,
			0xEF20CADA,0x36774C01,0xD07E9EFE,0x2BF11FB4,0x95DBDA4D,0xAE909198,0xEAAD8E71,0x6B93D5A0,
			0xD08ED1D0,0xAFC725E0,0x8E3C5B2F,0x8E7594B7,0x8FF6E2FB,0xF2122B64,0x8888B812,0x900DF01C,
			0x4FAD5EA0,0x688FC31C,0xD1CFF191,0xB3A8C1AD,0x2F2F2218,0xBE0E1777,0xEA752DFE,0x8B021FA1,
			0xE5A0CC0F,0xB56F74E8,0x18ACF3D6,0xCE89E299,0xB4A84FE0,0xFD13E0B7,0x7CC43B81,0xD2ADA8D9,
			0x165FA266,0x80957705,0x93CC7314,0x211A1477,0xE6AD2065,0x77B5FA86,0xC75442F5,0xFB9D35CF,
			0xEBCDAF0C,0x7B3E89A0,0xD6411BD3,0xAE1E7E49,0x00250E2D,0x2071B35E,0x226800BB,0x57B8E0AF,
			0x2464369B,0xF009B91E,0x5563911D,0x59DFA6AA,0x78C14389,0xD95A537F,0x207D5BA2,0x02E5B9C5,
			0x83260376,0x6295CFA9,0x11C81968,0x4E734A41,0xB3472DCA,0x7B14A94A,0x1B510052,0x9A532915,
			0xD60F573F,0xBC9BC6E4,0x2B60A476,0x81E67400,0x08BA6FB5,0x571BE91F,0xF296EC6B,0x2A0DD915,
			0xB6636521,0xE7B9F9B6,0xFF34052E,0xC5855664,0x53B02D5D,0xA99F8FA1,0x08BA4799,0x6E85076A
		}
		,
		{
			0x4B7A70E9,0xB5B32944,0xDB75092E,0xC4192623,0xAD6EA6B0,0x49A7DF7D,0x9CEE60B8,0x8FEDB266,
			0xECAA8C71,0x699A17FF,0x5664526C,0xC2B19EE1,0x193602A5,0x75094C29,0xA0591340,0xE4183A3E,
			0x3F54989A,0x5B429D65,0x6B8FE4D6,0x99F73FD6,0xA1D29C07,0xEFE830F5,0x4D2D38E6,0xF0255DC1,
			0x4CDD2086,0x8470EB26,0x6382E9C6,0x021ECC5E,0x09686B3F,0x3EBAEFC9,0x3C971814,0x6B6A70A1,
			0x687F3584,0x52A0E286,0xB79C5305,0xAA500737,0x3E07841C,0x7FDEAE5C,0x8E7D44EC,0x5716F2B8,
			0xB03ADA37,0xF0500C0D,0xF01C1F04,0x0200B3FF,0xAE0CF51A,0x3CB574B2,0x25837A58,0xDC0921BD,
			0xD19113F9,0x7CA92FF6,0x94324773,0x22F54701,0x3AE5E581,0x37C2DADC,0xC8B57634,0x9AF3DDA7,
			0xA9446146,0x0FD0030E,0xECC8C73E,0xA4751E41,0xE238CD99,0x3BEA0E2F,0x3280BBA1,0x183EB331,
			0x4E548B38,0x4F6DB908,0x6F420D03,0xF60A04BF,0x2CB81290,0x24977C79,0x5679B072,0xBCAF89AF,
			0xDE9A771F,0xD9930810,0xB38BAE12,0xDCCF3F2E,0x5512721F,0x2E6B7124,0x501ADDE6,0x9F84CD87,
			0x7A584718,0x7408DA17,0xBC9F9ABC,0xE94B7D8C,0xEC7AEC3A,0xDB851DFA,0x63094366,0xC464C3D2,
			0xEF1C1847,0x3215D908,0xDD433B37,0x24C2BA16,0x12A14D43,0x2A65C451,0x50940002,0x133AE4DD,
			0x71DFF89E,0x10314E55,0x81AC77D6,0x5F11199B,0x043556F1,0xD7A3C76B,0x3C11183B,0x5924A509,
			0xF28FE6ED,0x97F1FBFA,0x9EBABF2C,0x1E153C6E,0x86E34570,0xEAE96FB1,0x860E5E0A,0x5A3E2AB3,
			0x771FE71C,0x4E3D06FA,0x2965DCB9,0x99E71D0F,0x803E89D6,0x5266C825,0x2E4CC978,0x9C10B36A,
			0xC6150EBA,0x94E2EA78,0xA5FC3C53,0x1E0A2DF4,0xF2F74EA7,0x361D2B3D,0x1939260F,0x19C27960,
			0x5223A708,0xF71312B6,0xEBADFE6E,0xEAC31F66,0xE3BC4595,0xA67BC883,0xB17F37D1,0x018CFF28,
			0xC332DDEF,0xBE6C5AA5,0x65582185,0x68AB9802,0xEECEA50F,0xDB2F953B,0x2AEF7DAD,0x5B6E2F84,
			0x1521B628,0x29076170,0xECDD4775,0x619F1510,0x13CCA830,0xEB61BD96,0x0334FE1E,0xAA0363CF,
			0xB5735C90,0x4C70A239,0xD59E9E0B,0xCBAADE14,0xEECC86BC,0x60622CA7,0x9CAB5CAB,0xB2F3846E,
			0x648B1EAF,0x19BDF0CA,0xA02369B9,0x655ABB50,0x40685A32,0x3C2AB4B3,0x319EE9D5,0xC021B8F7,
			0x9B540B19,0x875FA099,0x95F7997E,0x623D7DA8,0xF837889A,0x97E32D77,0x11ED935F,0x16681281,
			0x0E358829,0xC7E61FD6,0x96DEDFA1,0x7858BA99,0x57F584A5,0x1B227263,0x9B83C3FF,0x1AC24696,
			0xCDB30AEB,0x532E3054,0x8FD948E4,0x6DBC3128,0x58EBF2EF,0x34C6FFEA,0xFE28ED61,0xEE7C3C73,
			0x5D4A14D9,0xE864B7E3,0x42105D14,0x203E13E0,0x45EEE2B6,0xA3AAABEA,0xDB6C4F15,0xFACB4FD0,
			0xC742F442,0xEF6ABBB5,0x654F3B1D,0x41CD2105,0xD81E799E,0x86854DC7,0xE44B476A,0x3D816250,
			0xCF62A1F2,0x5B8D2646,0xFC8883A0,0xC1C7B6A3,0x7F1524C3,0x69CB7492,0x47848A0B,0x5692B285,
			0x095BBF00,0xAD19489D,0x1462B174,0x23820E00,0x58428D2A,0x0C55F5EA,0x1DADF43E,0x233F7061,
			0x3372F092,0x8D937E41,0xD65FECF1,0x6C223BDB,0x7CDE3759,0xCBEE7460,0x4085F2A7,0xCE77326E,
			0xA6078084,0x19F8509E,0xE8EFD855,0x61D99735,0xA969A7AA,0xC50C06C2,0x5A04ABFC,0x800BCADC,
			0x9E447A2E,0xC3453484,0xFDD56705,0x0E1E9EC9,0xDB73DBD3,0x105588CD,0x675FDA79,0xE3674340,
			0xC5C43465,0x713E38D8,0x3D28F89E,0xF16DFF20,0x153E21E7,0x8FB03D4A,0xE6E39F2B,0xDB83ADF7
		}
		,
		{
			0xE93D5A68,0x948140F7,0xF64C261C,0x94692934,0x411520F7,0x7602D4F7,0xBCF46B2E,0xD4A20068,
			0xD4082471,0x3320F46A,0x43B7D4B7,0x500061AF,0x1E39F62E,0x97244546,0x14214F74,0xBF8B8840,
			0x4D95FC1D,0x96B591AF,0x70F4DDD3,0x66A02F45,0xBFBC09EC,0x03BD9785,0x7FAC6DD0,0x31CB8504,
			0x96EB27B3,0x55FD3941,0xDA2547E6,0xABCA0A9A,0x28507825,0x530429F4,0x0A2C86DA,0xE9B66DFB,
			0x68DC1462,0xD7486900,0x680EC0A4,0x27A18DEE,0x4F3FFEA2,0xE887AD8C,0xB58CE006,0x7AF4D6B6,
			0xAACE1E7C,0xD3375FEC,0xCE78A399,0x406B2A42,0x20FE9E35,0xD9F385B9,0xEE39D7AB,0x3B124E8B,
			0x1DC9FAF7,0x4B6D1856,0x26A36631,0xEAE397B2,0x3A6EFA74,0xDD5B4332,0x6841E7F7,0xCA7820FB,
			0xFB0AF54E,0xD8FEB397,0x454056AC,0xBA489527,0x55533A3A,0x20838D87,0xFE6BA9B7,0xD096954B,
			0x55A867BC,0xA1159A58,0xCCA92963,0x99E1DB33,0xA62A4A56,0x3F3125F9,0x5EF47E1C,0x9029317C,
			0xFDF8E802,0x04272F70,0x80BB155C,0x05282CE3,0x95C11548,0xE4C66D22,0x48C1133F,0xC70F86DC,
			0x07F9C9EE,0x41041F0F,0x404779A4,0x5D886E17,0x325F51EB,0xD59BC0D1,0xF2BCC18F,0x41113564,
			0x257B7834,0x602A9C60,0xDFF8E8A3,0x1F636C1B,0x0E12B4C2,0x02E1329E,0xAF664FD1,0xCAD18115,
			0x6B2395E0,0x333E92E1,0x3B240B62,0xEEBEB922,0x85B2A20E,0xE6BA0D99,0xDE720C8C,0x2DA2F728,
			0xD0127845,0x95B794FD,0x647D0862,0xE7CCF5F0,0x5449A36F,0x877D48FA,0xC39DFD27,0xF33E8D1E,
			0x0A476341,0x992EFF74,0x3A6F6EAB,0xF4F8FD37,0xA812DC60,0xA1EBDDF8,0x991BE14C,0xDB6E6B0D,
			0xC67B5510,0x6D672C37,0x2765D43B,0xDCD0E804,0xF1290DC7,0xCC00FFA3,0xB5390F92,0x690FED0B,
			0x667B9FFB,0xCEDB7D9C,0xA091CF0B,0xD9155EA3,0xBB132F88,0x515BAD24,0x7B9479BF,0x763BD6EB,
			0x37392EB3,0xCC115979,0x8026E297,0xF42E312D,0x6842ADA7,0xC66A2B3B,0x12754CCC,0x782EF11C,
			0x6A124237,0xB79251E7,0x06A1BBE6,0x4BFB6350,0x1A6B1018,0x11CAEDFA,0x3D25BDD8,0xE2E1C3C9,
			0x44421659,0x0A121386,0xD90CEC6E,0xD5ABEA2A,0x64AF674E,0xDA86A85F,0xBEBFE988,0x64E4C3FE,
			0x9DBC8057,0xF0F7C086,0x60787BF8,0x6003604D,0xD1FD8346,0xF6381FB0,0x7745AE04,0xD736FCCC,
			0x83426B33,0xF01EAB71,0xB0804187,0x3C005E5F,0x77A057BE,0xBDE8AE24,0x55464299,0xBF582E61,
			0x4E58F48F,0xF2DDFDA2,0xF474EF38,0x8789BDC2,0x5366F9C3,0xC8B38E74,0xB475F255,0x46FCD9B9,
			0x7AEB2661,0x8B1DDF84,0x846A0E79,0x915F95E2,0x466E598E,0x20B45770,0x8CD55591,0xC902DE4C,
			0xB90BACE1,0xBB8205D0,0x11A86248,0x7574A99E,0xB77F19B6,0xE0A9DC09,0x662D09A1,0xC4324633,
			0xE85A1F02,0x09F0BE8C,0x4A99A025,0x1D6EFE10,0x1AB93D1D,0x0BA5A4DF,0xA186F20F,0x2868F169,
			0xDCB7DA83,0x573906FE,0xA1E2CE9B,0x4FCD7F52,0x50115E01,0xA70683FA,0xA002B5C4,0x0DE6D027,
			0x9AF88C27,0x773F8641,0xC3604C06,0x61A806B5,0xF0177A28,0xC0F586E0,0x006058AA,0x30DC7D62,
			0x11E69ED7,0x2338EA63,0x53C2DD94,0xC2C21634,0xBBCBEE56,0x90BCB6DE,0xEBFC7DA1,0xCE591D76,
			0x6F05E409,0x4B7C0188,0x39720A3D,0x7C927C24,0x86E3725F,0x724D9DB9,0x1AC15BB4,0xD39EB8FC,
			0xED545578,0x08FCA5B5,0xD83D7CD3,0x4DAD0FC4,0x1E50EF5E,0xB161E6F8,0xA28514D9,0x6C51133C,
			0x6FD5C7E7,0x56E14EC4,0x362ABFCE,0xDDC6C837,0xD79A3234,0x92638212,0x670EFA8E,0x406000E0
		}
		,
		{
			0x3A39CE37,0xD3FAF5CF,0xABC27737,0x5AC52D1B,0x5CB0679E,0x4FA33742,0xD3822740,0x99BC9BBE,
			0xD5118E9D,0xBF0F7315,0xD62D1C7E,0xC700C47B,0xB78C1B6B,0x21A19045,0xB26EB1BE,0x6A366EB4,
			0x5748AB2F,0xBC946E79,0xC6A376D2,0x6549C2C8,0x530FF8EE,0x468DDE7D,0xD5730A1D,0x4CD04DC6,
			0x2939BBDB,0xA9BA4650,0xAC9526E8,0xBE5EE304,0xA1FAD5F0,0x6A2D519A,0x63EF8CE2,0x9A86EE22,
			0xC089C2B8,0x43242EF6,0xA51E03AA,0x9CF2D0A4,0x83C061BA,0x9BE96A4D,0x8FE51550,0xBA645BD6,
			0x2826A2F9,0xA73A3AE1,0x4BA99586,0xEF5562E9,0xC72FEFD3,0xF752F7DA,0x3F046F69,0x77FA0A59,
			0x80E4A915,0x87B08601,0x9B09E6AD,0x3B3EE593,0xE990FD5A,0x9E34D797,0x2CF0B7D9,0x022B8B51,
			0x96D5AC3A,0x017DA67D,0xD1CF3ED6,0x7C7D2D28,0x1F9F25CF,0xADF2B89B,0x5AD6B472,0x5A88F54C,
			0xE029AC71,0xE019A5E6,0x47B0ACFD,0xED93FA9B,0xE8D3C48D,0x283B57CC,0xF8D56629,0x79132E28,
			0x785F0191,0xED756055,0xF7960E44,0xE3D35E8C,0x15056DD4,0x88F46DBA,0x03A16125,0x0564F0BD,
			0xC3EB9E15,0x3C9057A2,0x97271AEC,0xA93A072A,0x1B3F6D9B,0x1E6321F5,0xF59C66FB,0x26DCF319,
			0x7533D928,0xB155FDF5,0x03563482,0x8ABA3CBB,0x28517711,0xC20AD9F8,0xABCC5167,0xCCAD925F,
			0x4DE81751,0x3830DC8E,0x379D5862,0x9320F991,0xEA7A90C2,0xFB3E7BCE,0x5121CE64,0x774FBE32,
			0xA8B6E37E,0xC3293D46,0x48DE5369,0x6413E680,0xA2AE0810,0xDD6DB224,0x69852DFD,0x09072166,
			0xB39A460A,0x6445C0DD,0x586CDECF,0x1C20C8AE,0x5BBEF7DD,0x1B588D40,0xCCD2017F,0x6BB4E3BB,
			0xDDA26A7E,0x3A59FF45,0x3E350A44,0xBCB4CDD5,0x72EACEA8,0xFA6484BB,0x8D6612AE,0xBF3C6F47,
			0xD29BE463,0x542F5D9E,0xAEC2771B,0xF64E6370,0x740E0D8D,0xE75B1357,0xF8721671,0xAF537D5D,
			0x4040CB08,0x4EB4E2CC,0x34D2466A,0x0115AF84,0xE1B00428,0x95983A1D,0x06B89FB4,0xCE6EA048,
			0x6F3F3B82,0x3520AB82,0x011A1D4B,0x277227F8,0x611560B1,0xE7933FDC,0xBB3A792B,0x344525BD,
			0xA08839E1,0x51CE794B,0x2F32C9B7,0xA01FBAC9,0xE01CC87E,0xBCC7D1F6,0xCF0111C3,0xA1E8AAC7,
			0x1A908749,0xD44FBD9A,0xD0DADECB,0xD50ADA38,0x0339C32A,0xC6913667,0x8DF9317C,0xE0B12B4F,
			0xF79E59B7,0x43F5BB3A,0xF2D519FF,0x27D9459C,0xBF97222C,0x15E6FC2A,0x0F91FC71,0x9B941525,
			0xFAE59361,0xCEB69CEB,0xC2A86459,0x12BAA8D1,0xB6C1075E,0xE3056A0C,0x10D25065,0xCB03A442,
			0xE0EC6E0E,0x1698DB3B,0x4C98A0BE,0x3278E964,0x9F1F9532,0xE0D392DF,0xD3A0342B,0x8971F21E,
			0x1B0A7441,0x4BA3348C,0xC5BE7120,0xC37632D8,0xDF359F8D,0x9B992F2E,0xE60B6F47,0x0FE3F11D,
			0xE54CDA54,0x1EDAD891,0xCE6279CF,0xCD3E7E6F,0x1618B166,0xFD2C1D05,0x848FD2C5,0xF6FB2299,
			0xF523F357,0xA6327623,0x93A83531,0x56CCCD02,0xACF08162,0x5A75EBB5,0x6E163697,0x88D273CC,
			0xDE966292,0x81B949D0,0x4C50901B,0x71C65614,0xE6C6C7BD,0x327A140A,0x45E1D006,0xC3F27B9A,
			0xC9AA53FD,0x62A80F00,0xBB25BFE2,0x35BDD2F6,0x71126905,0xB2040222,0xB6CBCF7C,0xCD769C2B,
			0x53113EC0,0x1640E3D3,0x38ABBD60,0x2547ADF0,0xBA38209C,0xF746CE76,0x77AFA1C5,0x20756060,
			0x85CBFE4E,0x8AE88DD8,0x7AAAF9B0,0x4CF9AA7E,0x1948C25C,0x02FB8A8C,0x01C36AE4,0xD6EBE1F9,
			0x90D4F869,0xA65CDEA0,0x3F09252D,0xC208E69F,0xB74E6132,0xCE77E25B,0x578FDFE3,0x3AC372E6
		}
	};
#pragma endregion
	/*
	char* p = "1111111111111111222222222222222233333333333333334444444444444444";
	int iplen = strlen(p);
	for(int i = 0; i < iplen; ++i){
		cb_space_algorithm::cb_blowfish bf;
		unsigned int len = 128;
		char pdes[128] = {0};
		bf.encrypt(p, i+1, pdes, len);

		char buf[1024] = {0};
		memcpy(buf, p, i + 1);
		printf("%d, %s\n", i+1, buf);
		//printf("%d, %s==%s\n", i+1, buf, pdes);

		cb_space_algorithm::cb_blowfish bf2;
		bf2.decrypt(pdes, len);
		memset(buf, 0, 1024);
		memcpy(buf, pdes, len);
		printf("-->%d, %s\n", len, buf);
	}
	*/
	class cb_blowfish
	{
		struct blowfish_key
		{
			unsigned int p[18];
			unsigned int s[4][256];
		};
	public:
		cb_blowfish(void)
		{
			unsigned char key[32] = 
			{
				0xF0,0xE1,0xD2,0xC3,0xB4,0xA5,0x96,0x87,
				0x78,0x69,0x5A,0x4B,0x3C,0x2D,0x1E,0x0F,
				0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
				0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77
			};
			blowfish_key_setup((unsigned char*)key, &m_key, 32);
		}
		virtual ~cb_blowfish(){}
	public:
		inline unsigned int getciphersize(unsigned int isrclen)
		{
			return (((isrclen + 8 - 1) & ~(8 - 1)) + 8);
		}
		int encrypt(const char* psrc, unsigned int isrclen, char* pdes, unsigned int& ideslen)
		{
			if(!psrc || isrclen <= 0 || !pdes)
				return -1;
			unsigned int ilen = getciphersize(isrclen);
			if(ideslen < ilen)
				return -2;

			if(psrc != pdes)
				memmove(pdes, psrc, isrclen);

			*((unsigned int*)&pdes[((isrclen + 8 - 1) & ~(8 - 1))]) = isrclen;

			for(unsigned int i = 0; i < ilen; i += 8)
				//aes_encrypt((unsigned char*)(&pdes[i]), (unsigned char*)(&pdes[i]), m_key_schedule, m_ikeysize);
				blowfish_encrypt((unsigned char*)(&pdes[i]), (unsigned char*)(&pdes[i]), &m_key);
			
			ideslen = ilen;

			return 0;
		}
		int decrypt(char* psrc, unsigned int& isrclen)
		{
			if(!psrc || isrclen <= 0){
				return -1;
			}
			if(isrclen % 8 != 0){
				return -2;
			}
			for(unsigned int i = 0; i < isrclen; i += 8)
				//aes_decrypt((unsigned char*)(&psrc[i]), (unsigned char*)(&psrc[i]), m_key_schedule, m_ikeysize);
				blowfish_decrypt((unsigned char*)(&psrc[i]), (unsigned char*)(&psrc[i]), &m_key);
			
			isrclen = *(unsigned int*)(&psrc[isrclen - 8]);

			return 0;
		}
	private:
		void blowfish_encrypt(const unsigned char in[], unsigned char out[], const blowfish_key *keystruct)
		{
			unsigned int l, r, t;

			l = (in[0] << 24) | (in[1] << 16) | (in[2] << 8) | (in[3]);
			r = (in[4] << 24) | (in[5] << 16) | (in[6] << 8) | (in[7]);

			l ^= keystruct->p[0];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[1];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[2];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[3];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[4];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[5];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[6];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[7];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[8];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[9];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[10];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[11];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[12];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[13];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[14];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[15];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t;

			r ^= keystruct->p[16];
			l ^= keystruct->p[17];

			out[0] = l >> 24;
			out[1] = l >> 16;
			out[2] = l >> 8;
			out[3] = l;
			out[4] = r >> 24;
			out[5] = r >> 16;
			out[6] = r >> 8;
			out[7] = r;
		}
		void blowfish_decrypt(const unsigned char in[], unsigned char out[], const blowfish_key *keystruct)
		{
			unsigned int l, r, t;

			l = (in[0] << 24) | (in[1] << 16) | (in[2] << 8) | (in[3]);
			r = (in[4] << 24) | (in[5] << 16) | (in[6] << 8) | (in[7]);

			l ^= keystruct->p[17];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[16];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[15];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[14];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[13];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[12];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[11];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[10];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[9];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[8];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[7];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[6];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[5];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[4];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[3];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t; t = l; l = r; r = t;

			l ^= keystruct->p[2];
			t = keystruct->s[0][(l) >> 24]; t += keystruct->s[1][((l) >> 16) & 0xff]; \
				t ^= keystruct->s[2][((l) >> 8) & 0xff]; t += keystruct->s[3][(l) & 0xff];
			r ^= t;

			r ^= keystruct->p[1];
			l ^= keystruct->p[0];

			out[0] = l >> 24;
			out[1] = l >> 16;
			out[2] = l >> 8;
			out[3] = l;
			out[4] = r >> 24;
			out[5] = r >> 16;
			out[6] = r >> 8;
			out[7] = r;
		}
		void blowfish_key_setup(const unsigned char user_key[], blowfish_key *keystruct, size_t len)
		{
			unsigned char block[8];
			int idx, idx2;

			memcpy(keystruct->p, p_perm, sizeof(unsigned int) * 18);
			memcpy(keystruct->s, s_perm, sizeof(unsigned int) * 1024);

			for(idx = 0, idx2 = 0; idx < 18; ++idx, idx2 += 4)
			{
				keystruct->p[idx] ^= (user_key[idx2 % len] << 24) | (user_key[(idx2+1) % len] << 16)
					| (user_key[(idx2+2) % len] << 8) | (user_key[(idx2+3) % len]);
			}

			memset(block, 0, 8);
			for(idx = 0; idx < 18; idx += 2)
			{
				blowfish_encrypt(block, block, keystruct);
				keystruct->p[idx] = (block[0] << 24) | (block[1] << 16) | (block[2] << 8) | block[3];
				keystruct->p[idx+1]=(block[4] << 24) | (block[5] << 16) | (block[6] << 8) | block[7];
			}

			for(idx = 0; idx < 4; ++idx)
			{
				for(idx2 = 0; idx2 < 256; idx2 += 2)
				{
					blowfish_encrypt(block, block, keystruct);
					keystruct->s[idx][idx2] = (block[0] << 24) | (block[1] << 16) |
						(block[2] << 8) | block[3];
					keystruct->s[idx][idx2+1] = (block[4] << 24) | (block[5] << 16) |
						(block[6] << 8) | block[7];
				}
			}
		}
	private:
		blowfish_key m_key;
	};

#pragma region md5
	#define cb_md5_s11 7
	#define cb_md5_s12 12
	#define cb_md5_s13 17
	#define cb_md5_s14 22
	#define cb_md5_s21 5
	#define cb_md5_s22 9
	#define cb_md5_s23 14
	#define cb_md5_s24 20
	#define cb_md5_s31 4
	#define cb_md5_s32 11
	#define cb_md5_s33 16
	#define cb_md5_s34 23
	#define cb_md5_s41 6
	#define cb_md5_s42 10
	#define cb_md5_s43 15
	#define cb_md5_s44 21

	#define cb_md5_f(x, y, z) (((x) & (y)) | ((~x) & (z)))
	#define cb_md5_g(x, y, z) (((x) & (z)) | ((y) & (~z)))
	#define cb_md5_h(x, y, z) ((x) ^ (y) ^ (z))
	#define cb_md5_i(x, y, z) ((y) ^ ((x) | (~z)))

	#define cb_md5_rotateleft(num, n) (((num) << (n)) | ((num) >> (32 - (n))))

	#define cb_md5_ff(a, b, c, d, x, s, ac){ \
	  (a) += cb_md5_f((b), (c), (d)) + (x) + ac; \
	  (a) = cb_md5_rotateleft((a), (s)); \
	  (a) += (b); \
	}
	#define cb_md5_gg(a, b, c, d, x, s, ac){ \
	  (a) += cb_md5_g((b), (c), (d)) + (x) + ac; \
	  (a) = cb_md5_rotateleft((a), (s)); \
	  (a) += (b); \
	}
	#define cb_md5_hh(a, b, c, d, x, s, ac){ \
	  (a) += cb_md5_h((b), (c), (d)) + (x) + ac; \
	  (a) = cb_md5_rotateleft((a), (s)); \
	  (a) += (b); \
	}
	#define cb_md5_ii(a, b, c, d, x, s, ac){ \
	  (a) += cb_md5_i((b), (c), (d)) + (x) + ac; \
	  (a) = cb_md5_rotateleft((a), (s)); \
	  (a) += (b); \
	}

	const unsigned char cb_md5_padding[64] = {0x80};
	const char cb_md5_hex_numbers[16] = 
	{
		'0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'a', 'b',
		'c', 'd', 'e', 'f'
	};
#pragma endregion
	/*
	printf("%s\n", cb_space_algorithm::cb_md5("").tostr().c_str());
	printf("%s\n", cb_space_algorithm::cb_md5("a").tostr().c_str());
	printf("%s\n", cb_space_algorithm::cb_md5("abc").tostr().c_str());
	printf("%s\n", cb_space_algorithm::cb_md5("message digest").tostr().c_str());
	printf("%s\n", cb_space_algorithm::cb_md5("abcdefghijklmnopqrstuvwxyz").tostr().c_str());
	printf("%s\n", cb_space_algorithm::cb_md5("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz").tostr().c_str());
	*/
	class cb_md5
	{
	public:
		cb_md5(const std::string& strtext):m_finished(false)
		{
			m_count[0] = m_count[1] = 0;
			m_state[0] = 0x67452301;
			m_state[1] = 0xefcdab89;
			m_state[2] = 0x98badcfe;
			m_state[3] = 0x10325476;
			init((unsigned char*)strtext.c_str(), strtext.length());
		}
		virtual ~cb_md5(){}
		std::string tostr()
		{
			const unsigned char* digest_ = getdigest();
			char str[64] = {0};
			for(size_t i = 0; i < 16; ++i)
			{
				str[i * 2 + 0] = cb_md5_hex_numbers[digest_[i] / 16];
				str[i * 2 + 1] = cb_md5_hex_numbers[digest_[i] % 16];
			}
			return std::string(str);
		}
	private:
		const unsigned char* getdigest(void)
		{
			if(!m_finished)
			{
				m_finished = true;

				unsigned char bits[8];
				unsigned int oldState[4];
				unsigned int oldCount[2];
				unsigned int index, padLen;

				memcpy(oldState, m_state, 16);
				memcpy(oldCount, m_count, 8);

				encode(m_count, bits, 8);

				index = (unsigned int)((m_count[0] >> 3) & 0x3f);
				padLen = (index < 56) ? (56 - index) : (120 - index);
				init(cb_md5_padding, padLen);

				init(bits, 8);

				encode(m_state, m_digest, 16);

				memcpy(m_state, oldState, 16);
				memcpy(m_count, oldCount, 8);
			}
			return m_digest;
		}
		void init(const unsigned char* input, size_t len)
		{

			unsigned int i, index, partLen;

			m_finished = false;

			index = (unsigned int)((m_count[0] >> 3) & 0x3f);

			if((m_count[0] += ((unsigned int)len << 3)) < ((unsigned int)len << 3)){
				++m_count[1];
			}
			m_count[1] += ((unsigned int)len >> 29);

			partLen = 64 - index;

			if(len >= partLen)
			{
				memcpy(&m_buffer[index], input, partLen);
				transform(m_buffer);

				for(i = partLen; i + 63 < len; i += 64){
					transform(&input[i]);
				}
				index = 0;
			}
			else{
				i = 0;
			}

			memcpy(&m_buffer[index], &input[i], len - i);
		}
		void transform(const unsigned char block[64])
		{
			unsigned int a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3], x[16];
			decode(block, x, 64);
			/* Round 1 */
			cb_md5_ff(a, b, c, d, x[ 0], cb_md5_s11, 0xd76aa478);
			cb_md5_ff(d, a, b, c, x[ 1], cb_md5_s12, 0xe8c7b756);
			cb_md5_ff(c, d, a, b, x[ 2], cb_md5_s13, 0x242070db);
			cb_md5_ff(b, c, d, a, x[ 3], cb_md5_s14, 0xc1bdceee);
			cb_md5_ff(a, b, c, d, x[ 4], cb_md5_s11, 0xf57c0faf);
			cb_md5_ff(d, a, b, c, x[ 5], cb_md5_s12, 0x4787c62a);
			cb_md5_ff(c, d, a, b, x[ 6], cb_md5_s13, 0xa8304613);
			cb_md5_ff(b, c, d, a, x[ 7], cb_md5_s14, 0xfd469501);
			cb_md5_ff(a, b, c, d, x[ 8], cb_md5_s11, 0x698098d8);
			cb_md5_ff(d, a, b, c, x[ 9], cb_md5_s12, 0x8b44f7af);
			cb_md5_ff(c, d, a, b, x[10], cb_md5_s13, 0xffff5bb1);
			cb_md5_ff(b, c, d, a, x[11], cb_md5_s14, 0x895cd7be);
			cb_md5_ff(a, b, c, d, x[12], cb_md5_s11, 0x6b901122);
			cb_md5_ff(d, a, b, c, x[13], cb_md5_s12, 0xfd987193);
			cb_md5_ff(c, d, a, b, x[14], cb_md5_s13, 0xa679438e);
			cb_md5_ff(b, c, d, a, x[15], cb_md5_s14, 0x49b40821);
			/* Round 2 */
			cb_md5_gg(a, b, c, d, x[ 1], cb_md5_s21, 0xf61e2562);
			cb_md5_gg(d, a, b, c, x[ 6], cb_md5_s22, 0xc040b340);
			cb_md5_gg(c, d, a, b, x[11], cb_md5_s23, 0x265e5a51);
			cb_md5_gg(b, c, d, a, x[ 0], cb_md5_s24, 0xe9b6c7aa);
			cb_md5_gg(a, b, c, d, x[ 5], cb_md5_s21, 0xd62f105d);
			cb_md5_gg(d, a, b, c, x[10], cb_md5_s22,  0x2441453);
			cb_md5_gg(c, d, a, b, x[15], cb_md5_s23, 0xd8a1e681);
			cb_md5_gg(b, c, d, a, x[ 4], cb_md5_s24, 0xe7d3fbc8);
			cb_md5_gg(a, b, c, d, x[ 9], cb_md5_s21, 0x21e1cde6);
			cb_md5_gg(d, a, b, c, x[14], cb_md5_s22, 0xc33707d6);
			cb_md5_gg(c, d, a, b, x[ 3], cb_md5_s23, 0xf4d50d87);
			cb_md5_gg(b, c, d, a, x[ 8], cb_md5_s24, 0x455a14ed);
			cb_md5_gg(a, b, c, d, x[13], cb_md5_s21, 0xa9e3e905);
			cb_md5_gg(d, a, b, c, x[ 2], cb_md5_s22, 0xfcefa3f8);
			cb_md5_gg(c, d, a, b, x[ 7], cb_md5_s23, 0x676f02d9);
			cb_md5_gg(b, c, d, a, x[12], cb_md5_s24, 0x8d2a4c8a);
			/* Round 3 */
			cb_md5_hh(a, b, c, d, x[ 5], cb_md5_s31, 0xfffa3942);
			cb_md5_hh(d, a, b, c, x[ 8], cb_md5_s32, 0x8771f681);
			cb_md5_hh(c, d, a, b, x[11], cb_md5_s33, 0x6d9d6122);
			cb_md5_hh(b, c, d, a, x[14], cb_md5_s34, 0xfde5380c);
			cb_md5_hh(a, b, c, d, x[ 1], cb_md5_s31, 0xa4beea44);
			cb_md5_hh(d, a, b, c, x[ 4], cb_md5_s32, 0x4bdecfa9);
			cb_md5_hh(c, d, a, b, x[ 7], cb_md5_s33, 0xf6bb4b60);
			cb_md5_hh(b, c, d, a, x[10], cb_md5_s34, 0xbebfbc70);
			cb_md5_hh(a, b, c, d, x[13], cb_md5_s31, 0x289b7ec6);
			cb_md5_hh(d, a, b, c, x[ 0], cb_md5_s32, 0xeaa127fa);
			cb_md5_hh(c, d, a, b, x[ 3], cb_md5_s33, 0xd4ef3085);
			cb_md5_hh(b, c, d, a, x[ 6], cb_md5_s34,  0x4881d05);
			cb_md5_hh(a, b, c, d, x[ 9], cb_md5_s31, 0xd9d4d039);
			cb_md5_hh(d, a, b, c, x[12], cb_md5_s32, 0xe6db99e5);
			cb_md5_hh(c, d, a, b, x[15], cb_md5_s33, 0x1fa27cf8);
			cb_md5_hh(b, c, d, a, x[ 2], cb_md5_s34, 0xc4ac5665);
			/* Round 4 */
			cb_md5_ii(a, b, c, d, x[ 0], cb_md5_s41, 0xf4292244);
			cb_md5_ii(d, a, b, c, x[ 7], cb_md5_s42, 0x432aff97);
			cb_md5_ii(c, d, a, b, x[14], cb_md5_s43, 0xab9423a7);
			cb_md5_ii(b, c, d, a, x[ 5], cb_md5_s44, 0xfc93a039);
			cb_md5_ii(a, b, c, d, x[12], cb_md5_s41, 0x655b59c3);
			cb_md5_ii(d, a, b, c, x[ 3], cb_md5_s42, 0x8f0ccc92);
			cb_md5_ii(c, d, a, b, x[10], cb_md5_s43, 0xffeff47d);
			cb_md5_ii(b, c, d, a, x[ 1], cb_md5_s44, 0x85845dd1);
			cb_md5_ii(a, b, c, d, x[ 8], cb_md5_s41, 0x6fa87e4f);
			cb_md5_ii(d, a, b, c, x[15], cb_md5_s42, 0xfe2ce6e0);
			cb_md5_ii(c, d, a, b, x[ 6], cb_md5_s43, 0xa3014314);
			cb_md5_ii(b, c, d, a, x[13], cb_md5_s44, 0x4e0811a1);
			cb_md5_ii(a, b, c, d, x[ 4], cb_md5_s41, 0xf7537e82);
			cb_md5_ii(d, a, b, c, x[11], cb_md5_s42, 0xbd3af235);
			cb_md5_ii(c, d, a, b, x[ 2], cb_md5_s43, 0x2ad7d2bb);
			cb_md5_ii(b, c, d, a, x[ 9], cb_md5_s44, 0xeb86d391);
			m_state[0] += a;
			m_state[1] += b;
			m_state[2] += c;
			m_state[3] += d;
		}
		void encode(const unsigned int* input, unsigned char* output, size_t length)
		{
			for(size_t i = 0, j = 0; j < length; ++i, j += 4)
			{
				output[j    ] = (unsigned char)( input[i]        & 0xff);
				output[j + 1] = (unsigned char)((input[i] >>  8) & 0xff);
				output[j + 2] = (unsigned char)((input[i] >> 16) & 0xff);
				output[j + 3] = (unsigned char)((input[i] >> 24) & 0xff);
			}
		}
		void decode(const unsigned char* input, unsigned int* output, size_t length)
		{
			for(size_t i = 0, j = 0; j < length; ++i, j += 4)
			{
				output[i] = ((unsigned int)input[j]) | (((unsigned int)input[j + 1]) << 8) |
					(((unsigned int)input[j + 2]) << 16) | (((unsigned int)input[j + 3]) << 24);
			}
		}
	private:
		bool m_finished;
		unsigned int m_state[4];
		unsigned int m_count[2];
		unsigned char m_buffer[64];
		unsigned char m_digest[16];
	};

#pragma region base64
	const char base64_table[] = 
	{
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
		'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
		'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/', '\0'
	};
	const short base64_reverse_table[256] = 
	{
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -1, -2, -2, -1, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
		-1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 62, -2, -2, -2, 63,
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -2, -2, -2, -2, -2, -2,
		-2,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -2, -2, -2, -2, -2,
		-2, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2
	};
#pragma endregion
	/*
	const char test[] =
	"Redistribution and use in source and binary forms, with or without\n";
	
	printf("message:%d\n", strlen(test));
	printf("%s\n\n", test);

	cb_space_algorithm::cb_base64 b64;

	std::string penstr;
	b64.base64_encode(test, strlen(test), penstr);
	printf("encoded base64: %d\n%s\n\n", penstr.length(), penstr.c_str());

	char buf[4096] = {0}; char* p1 = buf;
	unsigned int idelen = 4096;
	if(penstr.length() > 4096){
		p1 = new char[idelen];
		memset(p1, 0, idelen);
		idelen = penstr.length();
	}
	char* pdestr = b64.base64_decode(penstr.c_str(), penstr.length(), p1, idelen);
	if(pdestr){
		printf("decoded again: %d\n%s\n\n", idelen, pdestr);
		if(pdestr != p1){
			delete pdestr, pdestr = 0;
		}
	}
	*/
	class cb_base64
	{
	public:
		cb_base64(){}
		virtual ~cb_base64(){}
	public:
		void base64_encode(const char* psrc, unsigned int isrclen, std::string& str)
		{
			if(!psrc || isrclen <= 0)
				return ;
			char buf[4096] = {0}; char* p = buf;
			int ilen = (((isrclen + 2) / 3) * 4) * sizeof(char);
			if(ilen >= 4096)
			{
				p = new(std::nothrow) char[ilen];
				if(!p)
					return ;
				memset(p, 0, ilen);
			}
			char* pold = p;
			while(isrclen > 2)
			{
				*p++ = base64_table[psrc[0] >> 2];
				*p++ = base64_table[((psrc[0] & 0x03) << 4) + (psrc[1] >> 4)];
				*p++ = base64_table[((psrc[1] & 0x0f) << 2) + (psrc[2] >> 6)];
				*p++ = base64_table[psrc[2] & 0x3f];
				psrc += 3;
				isrclen -= 3;
			}

			if(isrclen != 0)
			{
				*p++ = base64_table[psrc[0] >> 2];
				if(isrclen > 1)
				{
					*p++ = base64_table[((psrc[0] & 0x03) << 4) + (psrc[1] >> 4)];
					*p++ = base64_table[(psrc[1] & 0x0f) << 2];
					*p++ = '=';
				}
				else{
					*p++ = base64_table[(psrc[0] & 0x03) << 4];
					*p++ = '=';
					*p++ = '=';
				}
			}
			str = (pold);
			if(pold != buf){
				delete p, p = 0;
			}
			return ;
		}
		char* base64_decode(const char* psrc, unsigned int isrclen, char* pdes, unsigned int& ideslen)
		{
			int ch, i = 0, j = 0, k;
			char* p = pdes;
			if(ideslen < isrclen)
			{
				p = new(std::nothrow) char[isrclen];
				if(!p)
					return 0;
				memset(p, 0, isrclen);
				ideslen = isrclen;
			}
			while((ch = *psrc++) != '\0' && isrclen-- > 0)
			{
				if(ch == '=')
				{
					if(*psrc != '=' && (i % 4) == 1)
					{
						if(p != pdes){
							delete p, p = 0;
						}
						return 0;
					}
					continue;
				}
				ch = base64_reverse_table[ch];

				if((1 && ch < 0) || ch == -1)
					continue;
				else if(ch == -2)
				{
					if(p != pdes){
						delete p, p = 0;
					}
					return 0;
				}

				switch(i % 4)
				{
				case 0:
					p[j] = ch << 2;
					break;
				case 1:
					p[j++] |= ch >> 4;
					p[j] = (ch & 0x0f) << 4;
					break;
				case 2:
					p[j++] |= ch >>2;
					p[j] = (ch & 0x03) << 6;
					break;
				case 3:
					p[j++] |= ch;
					break;
				}
				i++;
			}

			k = j;

			if(ch == '=')
			{
				switch(i % 4)
				{
				case 1:
					if(p != pdes){
						delete p, p = 0;
					}
					return 0;
				case 2:
					k++;
				case 3:
					p[k] = 0;
				}
			}
			ideslen = j;
			return p;
		}
	};
};

namespace cb_space_algorithm
{
	/*
	int a[] = {1, 5, 20, 3, 6, 2, 9, 4, 10, 7, 8, 14, 13, 16, 11, 19, 12, 15, 18, 17, 21};
	cb_space_algorithm::cb_sort::quick_sort(a, sizeof(a)/sizeof(a[0]));
	*/
	/*
	int cmp_dec(int& i, int& j)
	{
		return i < j;
	}
	int a[] = {1, 5, 20, 3, 6, 2, 9, 4, 10, 7, 8, 14, 13, 16, 11, 19, 12, 15, 18, 17};
	cb_space_algorithm::cb_sort::heap_sort(a, 20, cmp_dec);//default cmp is inc
	*/
	/*
	int a[] = {1, 5, 20, 3, 6, 2, 9, 4, 10, 7, 8, 14, 13, 16, 11, 19, 12, 15, 18, 17, 21};
	cb_space_algorithm::cb_sort::merge_sort(a, sizeof(a)/sizeof(a[0]));
	*/
	class cb_sort
	{
	public:
		template<typename T>static void quick_sort(T* pdata, int isize)
		{
			quicksort(pdata, 0, isize - 1);
		}
		/* void* ==> int (*)(T&, T&); */
		template<typename T>static void heap_sort(T* pdata, int isize, void* cmp = 0)
		{
			int i = isize / 2 - 1;
			for(; i >= 0; --i)
				adjust_heap(pdata, i, isize, cmp);
			i = isize;
			for(; i > 1; --i)
			{
				T t = pdata[0]; pdata[0] = pdata[i - 1]; pdata[i-1] = t;
				adjust_heap(pdata, 0, i - 1, cmp);
			}
		}
		template<typename T>static void merge_sort(T* pdata, int isize)
		{
			mergesort(pdata, 0, isize - 1);
		}
	private:
		template<typename T>static void quicksort(T* pdata, int ileft, int iright)
		{
			int ipivot = (ileft + iright)/2;
			int npivot = pdata[ipivot];
			for(int i = ileft, j = iright; i <= j; )
			{
				while(pdata[i] <= npivot && i <= ipivot) i++;
				if(i <= ipivot)
				{
					pdata[ipivot] = pdata[i];
					ipivot = i;
				}
				while(pdata[j] >= npivot && j >= ipivot) j--;
				if(j >= ipivot)
				{
					pdata[ipivot] = pdata[j];
					ipivot = j;
				}
			}

			pdata[ipivot] = npivot;

			if(ipivot - ileft > 1)
				quicksort(pdata, ileft, ipivot - 1);

			if(iright - ipivot > 1)
				quicksort(pdata, ipivot + 1, iright);
		}
		template<typename T>static void adjust_heap(T* pdata, int i, int num, void* cmp)
		{
			int left = (2*(i)+1); int right = (2*(i)+2);
			if(left < num){
				if((right < num) && 
					(cmp ? (((int (*)(T&, T&))cmp)(pdata[right], pdata[left]) > 0) 
						: (pdata[right] > pdata[left])))
				{
					if(cmp ? (((int (*)(T&, T&))cmp)(pdata[right], pdata[i]) > 0) : (pdata[right] > pdata[i])){
						T t = pdata[i]; pdata[i] = pdata[right]; pdata[right] = t;
						adjust_heap(pdata, right, num, cmp);
					}
				}
				else{
					if(cmp ? (((int (*)(T&, T&))cmp)(pdata[left], pdata[i]) > 0) : (pdata[left] > pdata[i])){
						T t = pdata[i]; pdata[i] = pdata[left]; pdata[left] = t;
						adjust_heap(pdata, left, num, cmp);
					}
				}
			}
		}
		template<typename T>static void mergesort(T* pdata, int begin, int end)
		{
			if(begin < end)
			{
				int middle = (begin + end)/2;
				mergesort(pdata, begin, middle);
				mergesort(pdata, middle + 1, end);
				merge(pdata, begin, middle, end);
			}
		}
		template<typename T>static void merge(T* pdata, int begin, int middle, int end)
		{
			int n = end - begin + 1, m = middle - begin + 1, isize = n, i = 0, j = m, k = begin;
			T tarr[1024] = {0}; T* ptarr = tarr;
			if(isize > 1024){
				ptarr = new(std::nothrow) T[isize];
				if(!ptarr)
					return ;
				memset(ptarr, 0, sizeof(T) * isize);
			}
			for(; i < isize; ++i)
				ptarr[i] = pdata[begin + i];
			for(i = 0; k <= end; ++k)
			{
				if(i == m){
					pdata[k] = ptarr[j++];
				}
				else if(j == n){
					pdata[k] = ptarr[i++];
				}
				else
					ptarr[i] > ptarr[j] ? pdata[k] = ptarr[j++] : pdata[k] = ptarr[i++];
			}
			if(ptarr != tarr)
				delete[] ptarr, ptarr = 0;
		}
	};

	//AVL-Tree
};

namespace cb_space_thread
{
	//auto / no signal
#ifndef WIN32
#include <sys/sem.h> //struct sembuf
#endif
	class cb_thread_0_0;
	struct procparam
	{
		procparam():_proc(0),_procparam(0),_pthis(0)
		{
#ifdef WIN32
			_hevent = 0;
#else
			_hevent = -1;
			_istop = 0;
#endif
			createevent();

		}
		virtual~procparam(){_proc = 0; _procparam = 0; _pthis = 0; destoryevent();}
		int createevent(void)
		{
#ifdef WIN32
			if(_hevent){
				int i = (::CloseHandle(_hevent) ? 0 : -1); _hevent = 0; return i;
			}
			return (_hevent = ::CreateEvent(0, 0/*1:need manualreset*/, 0/*0:no signal*/, 0)) ? 0 : -1;
#else
			_hevent = semget(IPC_PRIVATE, 1, IPC_CREAT|S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
			if(_hevent == -1)
				return -2;
			return 0;
#endif
		}
		int destoryevent(void)
		{
#ifdef WIN32
			if(_hevent){
				int i = (::CloseHandle(_hevent) ? 0 : -1); _hevent = 0; return i;
			}
			return 0;
#else
			_hevent = -1;
#endif
		}
	public:
		pvoid_proc_pvoid _proc;
		void* _procparam;
		cb_thread_0_0* _pthis;
#ifdef WIN32
		HANDLE _hevent;
#else
		int _hevent;
		volatile int _istop;
#endif
	};
#ifndef WIN32
	void* __cb_thread_proc__(void* pprocparam)
	{
		procparam* pth = (procparam*)pprocparam;
		while(cb_lockcompareexchange(pth->_istop, 0, 0) == 0)
		{
			struct sembuf sem;
			sem.sem_num = 0;
			sem.sem_op  = -1;
			sem.sem_flg = 0;
			if(semop(pth->_hevent, &sem, 1) == -1)
				break;
			pth->_proc(pth->_procparam);
		}
		cb_lockexchange(pth->_istop, 2);
		return 0;
	}
#endif
	/*
	void* proc(void*){}

	cb_space_thread::cb_thread_0_0 th;
	th.start(proc, 0);
	int j = 0;
	while(j++ < 10){
		cb_sleep(1000);
		th.setevent();
	}
	th.stop();
	*/
	class cb_thread_0_0
	{
	public:
		virtual ~cb_thread_0_0(){stop();}
	public:
		int start(pvoid_proc_pvoid proc, void* pprocparam)
		{
			if(!proc)
				return -1;

			m_procparam._proc = proc;
			m_procparam._procparam = pprocparam;
			m_procparam._pthis = this;

			cb_thread_fd tfd;
			cb_thread_create(tfd, __cb_thread_proc__, &m_procparam);
			if(cb_thread_fail(tfd))
				return -2;

			return 0;
		}
		int stop(void)
		{
#ifdef WIN32
			return (m_procparam.destoryevent() | setevent());
#else
			cb_lockexchange(m_procparam._istop, 1);
			while(cb_lockcompareexchange(m_procparam._istop, 2, 2) != 2){
				cb_sleep(100);
			}
			return setevent();
#endif
		}
		int setevent(void)
		{
#ifdef WIN32
			return ::SetEvent(m_procparam._hevent) ? 0 : -1;
#else
			struct sembuf sem;
			sem.sem_num = 0;
			sem.sem_op  = 1;
			sem.sem_flg = 0;
			if(semop(m_procparam._hevent, &sem, 1) == -1){
				return -2;
			}
			return 0;
#endif
		}
	private:
#ifdef WIN32
		static unsigned int _stdcall __cb_thread_proc__(void* pprocparam)
		{
			procparam* pth = (procparam*)pprocparam;
			while(1)
			{
				switch(WaitForSingleObject(pth->_hevent, -1))
				{
				case 0:
					pth->_proc(pth->_procparam); break;
				case 258: break;//time over
				//case -1: iret = 1;
				//case 128: iret = 2;
				default:
					return (unsigned int)-1;
				}
			}
			return 0;
		}
#endif
	private:
		procparam m_procparam;
	};
};

class boob_log
{
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

boob_log  cb_log;

namespace cb_space_server
{
#ifdef WIN32
	class businessdispenseI
	{
	public:
		virtual int handledatahead(char* pheadbuf, int headsize)
		{

			return 0;
		}
	};

	struct node;

	struct elem
	{
		cb_lock_ul   m_nlockelem;
		struct elem* m_pprevelem;
		struct elem* m_pnextelem;
		struct node* m_pbelongnode;
	};

	class timeoutcallback_i
	{
	public:
		virtual int timeout_callback(void*) = 0;
	};

	struct node
	{
		cb_lock_ul   m_nlocknode;
		struct node* m_pprevnode;
		struct node* m_pnextnode;
		struct elem* m_pelemhead;
	};

	typedef class xxx_abc
	{
		typedef xxx_abc __cb_class__;
	public:
		xxx_abc():m_del(0),m_pnodehead(0),m_tse(0),m_timeout_callback(0){}
		~xxx_abc()
		{
			if(m_tse)
			{
				timeKillEvent(m_tse); m_tse = 0;
				struct node* bhead = 0;
				while(1)
				{
					while(cb_lockcompareexchange(m_pnodehead->m_nlocknode, 1, 0) == 1);
					while(m_pnodehead->m_pelemhead)
					{
						//任何elem的操作都必须先lock node,再操作elem,如果node为空,那么可以直接操作elem
						//所以此处可以不做elem的lock
						struct elem* pelem = m_pnodehead->m_pelemhead;
						m_pnodehead->m_pelemhead = pelem->m_pnextelem;
						pelem->m_pnextelem = pelem->m_pprevelem = 0;
						pelem->m_pbelongnode = 0;

						m_timeout_callback->timeout_callback(pelem);
					}
					struct node* pnode = m_pnodehead;
					m_pnodehead = m_pnodehead->m_pnextnode;
					if(!bhead)
						bhead = pnode;
					else if(bhead == m_pnodehead){
						cb_lockexchange(pnode->m_nlocknode, 0);
						break;
					}
					cb_lockexchange(pnode->m_nlocknode, 0);
				}
			}

			m_pnodehead = 0;
			if(m_del)
				delete[] m_del, m_del = 0;
		}
	public:
		int start(timeoutcallback_i* pcallback, unsigned int udelay = 1000, unsigned int usize = 10)
		{
			if(!pcallback)
				return -1;
			m_timeout_callback = pcallback;

			if(!m_pnodehead)
			{
				char* p = new(std::nothrow)char[sizeof(struct node) * (usize <= 1 ? usize = 10 : usize)];
				if(!p)
					return -1;
				memset(p, 0, sizeof(struct node) * usize);

				m_del = p;
				m_pnodehead = (struct node*)p;

				((struct node*)p)[0].m_pprevnode = &((struct node*)p)[usize - 1];
				((struct node*)p)[usize - 1].m_pnextnode = (struct node*)p;
				for(unsigned int i = 0; i < usize - 1; ++i)
				{
					((struct node*)p)[i].m_pnextnode = &(((struct node*)p)[i + 1]);
					((struct node*)p)[i + 1].m_pprevnode = &(((struct node*)p)[i]);
				}
			}

			m_tse = timeSetEvent(udelay, 1, proc, (DWORD_PTR)this, 1);
			if(m_tse == 0)
				return -4;
			return 0;
		}

		int add(struct elem* pelem)
		{
			//可以确定pelem->m_pbelongnode为空,否则业务异常
			if(pelem->m_pbelongnode)
				return -1;
			struct node* pnode = m_pnodehead->m_pprevnode;
			while(cb_lockcompareexchange(pnode->m_nlocknode, 1, 0) == 1) pnode = m_pnodehead->m_pprevnode;
			pelem->m_pbelongnode = pnode;
			if(pnode->m_pelemhead)
			{
				pnode->m_pelemhead->m_pprevelem = pelem;
				pelem->m_pnextelem = pnode->m_pelemhead;
			}
			pnode->m_pelemhead = pelem;
			cb_lockexchange(pnode->m_nlocknode, 0);
			return 0;
		}

		int del(struct elem* pelem)
		{
			struct node* pnode = pelem->m_pbelongnode;
			if(!pnode)
			{
				//已经被处理掉了,请结合具体业务处理线程分析
				return -1;
			}
			while(cb_lockcompareexchange(pnode->m_nlocknode, 1, 0) == 1);
			{
				if(pnode == pelem->m_pbelongnode)
				{
					if(pnode->m_pelemhead == pelem)
					{
						pnode->m_pelemhead = pelem->m_pnextelem;
						if(pelem->m_pnextelem)
							pelem->m_pnextelem->m_pprevelem = 0;
					}
					else{
						pelem->m_pprevelem->m_pnextelem = pelem->m_pnextelem;
						if(pelem->m_pnextelem)
							pelem->m_pnextelem->m_pprevelem = pelem->m_pprevelem;
					}
					pelem->m_pnextelem = pelem->m_pprevelem = 0;
					pelem->m_pbelongnode = 0;
				}
				else{
					if(pelem->m_pbelongnode)
					{
						//已经切换了所属节点,具体是否为异常,需要结合业务处理线程
						cb_lockexchange(pnode->m_nlocknode, 0);
						return -3;
					}
					else{
						//等同于if(!pnode), 只不过是在这之后且在while(cb_lockcompareexchange锁定之前,已经处理掉了
						cb_lockexchange(pnode->m_nlocknode, 0);
						return -2;
					}
				}
			}
			cb_lockexchange(pnode->m_nlocknode, 0);
			return 0;
		}

		int exchange_hb(struct elem* pelem)
		{
			struct node* pnode = pelem->m_pbelongnode;
			if(!pnode)
			{
				//已经被处理掉了,请结合具体业务处理线程分析
				return -1;
			}
			if(pnode == m_pnodehead->m_pprevnode)
			{
				return 0;
			}
			while(cb_lockcompareexchange(pnode->m_nlocknode, 1, 0) == 1);
			{
				if(pnode == pelem->m_pbelongnode)
				{
					if(pnode->m_pelemhead == pelem)
					{
						pnode->m_pelemhead = pelem->m_pnextelem;
						if(pelem->m_pnextelem)
							pelem->m_pnextelem->m_pprevelem = 0;
					}
					else{
						pelem->m_pprevelem->m_pnextelem = pelem->m_pnextelem;
						if(pelem->m_pnextelem)
							pelem->m_pnextelem->m_pprevelem = pelem->m_pprevelem;
					}
					pelem->m_pnextelem = pelem->m_pprevelem = 0;
					pelem->m_pbelongnode = 0;
					cb_lockexchange(pnode->m_nlocknode, 0);

					//重置心跳
					struct node* pprevnode = m_pnodehead->m_pprevnode;
					while(cb_lockcompareexchange(pprevnode->m_nlocknode, 1, 0) == 1) pprevnode = m_pnodehead->m_pprevnode;
					pelem->m_pbelongnode = pprevnode;
					if(pprevnode->m_pelemhead)
					{
						pprevnode->m_pelemhead->m_pprevelem = pelem;
						pelem->m_pnextelem = pprevnode->m_pelemhead;
					}
					pprevnode->m_pelemhead = pelem;
					cb_lockexchange(pprevnode->m_nlocknode, 0);
				}
				else{
					if(pelem->m_pbelongnode)
					{
						//已经切换了所属节点,具体是否为异常,需要结合业务处理线程
						cb_lockexchange(pnode->m_nlocknode, 0);
						return -3;
					}
					else{
						//等同于if(!pnode), 只不过是在这之后且在while(cb_lockcompareexchange锁定之前,已经处理掉了
						cb_lockexchange(pnode->m_nlocknode, 0);
						return -2;
					}
				}
			}
			return 0;
		}
	private:
		static void __stdcall proc(unsigned int , unsigned int , unsigned long pparams, unsigned long , unsigned long)
		{
			__cb_class__* pclass = (__cb_class__*)pparams;
			pclass->handle();
			return ;
		}

		int handle(void)
		{
			while(cb_lockcompareexchange(m_pnodehead->m_nlocknode, 1, 0) == 1);
			while(m_pnodehead->m_pelemhead)
			{
				//任何elem的操作都必须先lock node,再操作elem,如果node为空,那么可以直接操作elem, 所以此处可以不做elem的lock
				struct elem* pelem = m_pnodehead->m_pelemhead;
				m_pnodehead->m_pelemhead = pelem->m_pnextelem;
				pelem->m_pnextelem = pelem->m_pprevelem = 0;
				m_timeout_callback->timeout_callback(pelem);
				pelem->m_pbelongnode = 0;
			}
			struct node* pnode = m_pnodehead;
			m_pnodehead = m_pnodehead->m_pnextnode;
			cb_lockexchange(pnode->m_nlocknode, 0);
			return 0;
		}
	private:
		char* m_del;//申请的内存(需要删除)
		struct node* m_pnodehead;//超时循环链表头
		unsigned int m_tse;//超时句柄

		////////////////超时数据回调//////////////
		timeoutcallback_i* m_timeout_callback;
	}__cb_class__;
	
	typedef enum IO_TYPE
	{
		IO_TYPE_NULL,
		IO_TYPE_DOFR,
		IO_TYPE_RECV,
		IO_TYPE_POST,
	}IO_TYPE;

	typedef struct DATA_HEAD
	{
		int _DataLength_;
		char N[28];
	}DATA_HEAD, *PDATA_HEAD;

	typedef struct IO_DATA
	{
		char* _pIOData_;
		int _IODataLen_;
		int _IODataIndex_;
	}IO_DATA, *PIO_DATA;

	typedef struct SOCK_CONTEXT : elem
	{
		int AddIOCount(void)
		{
			int iret = 0;
			while(cb_lockcompareexchange(_IOC_Lock_, 1, 0) == 1);
			iret = (++_IOC_Count_);
			cb_lockexchange(_IOC_Lock_, 0);
			return iret;
		}
		int DelIOCount(void)
		{
			int iret = 0;
			while(cb_lockcompareexchange(_IOC_Lock_, 1, 0) == 1);
			if(_IOC_Count_ > 0){
				iret = (--_IOC_Count_);
			}
			else{
				iret = -1;//错误,代表取得此值的线程不需要处理此 SOCKCONTEXT
			}
			cb_lockexchange(_IOC_Lock_, 0);
			return iret;
		}
		int GetIOCount(void)
		{
			int iret = 0;
			while(cb_lockcompareexchange(_IOC_Lock_, 1, 0) == 1);
			iret = _IOC_Count_;
			cb_lockexchange(_IOC_Lock_, 0);
			return iret;
		}
		cb_lock_ul _IOC_Count_;
		cb_lock_ul _IOC_Lock_;

		int AddRecvBuf(char* pRecvBuf, int nRecvSize, bool bNew = false, int nRecvDataLen = 0, char** pDelBuf = 0)
		{
			if(bNew)
			{
				if(nRecvDataLen <= 0 || !pDelBuf || nRecvSize > nRecvDataLen)
				{
					return -1;
				}
				*pDelBuf = _IORecvData_._pIOData_;
				_IORecvData_._pIOData_		= pRecvBuf;
				_IORecvData_._IODataLen_	= nRecvDataLen;
				_IORecvData_._IODataIndex_	= nRecvSize;
			}
			else{
				if(nRecvSize + _IORecvData_._IODataIndex_ > nRecvDataLen)
				{
					return -2;
				}
				memcpy(&_IORecvData_._pIOData_[_IORecvData_._IODataIndex_], pRecvBuf, nRecvSize);
				_IORecvData_._IODataIndex_ += nRecvSize;
			}
			return 0;
		}
		IO_DATA _IORecvData_;
		IO_DATA _IOSendData_;
// 		int AddRecvBuf(char* pRecvBuf, int nBufLen)
// 		{
// 			if(!pRecvBuf || nBufLen <= 0){
// 				return -1;
// 			}
// 			int iRet = 0;
// 			while(cb_lockcompareexchange(_IOData_Lock_, 1, 0) == 1);
// 			if(_pIODataRecvHead_)
// 			cb_lockexchange(_IOData_Lock_, 0);
// 			return iRet;
// 		}
// 		int AddIOData(char* pData, int nDataLen)
// 		{
// 			if(!pData || nDataLen <= 0)
// 			{
// 				return -1;
// 			}
// 			int iRet = 0;
// 			while(cb_lockcompareexchange(_IOData_Lock_, 1, 0) == 1);
// 			if(!_pIODataRecvHead_){
// 
// 			}
// 			cb_lockexchange(_IOData_Lock_, 0);
// 			return iRet;
// 		}
// 		cb_lock_ul _IOData_Lock_;
// 		IO_DATA* _pIODataRecvHead_;
// 		IO_DATA* _pIODataSendHead_;


		SOCKADDR_IN	_SockC_ClientAddr_;
		SOCKET		_SockC_Socket_;
	}SOCK_CONTEXT,*PSOCK_CONTEXT;

	typedef struct LISTEN_IO_CONTEXT
	{
		OVERLAPPED	_OL_;
		cb_socket	_Listen_IO_Sock_;
		WSABUF		_ListenIOBuf_;
	}LISTEN_IO_CONTEXT,*PLISTEN_IO_CONTEXT;

	typedef struct IO_CONTEXT
	{
		OVERLAPPED	_OL_;

		IO_TYPE		_IO_Type_;
		WSABUF		_IOBuf_;
	}IO_CONTEXT,*PIO_CONTEXT;

	class iocp_coderorz : public timeoutcallback_i
	{
		iocp_coderorz():m_hiocp(0),m_ListenSock(~0),m_lpfnacceptex(0),m_lpfngetacceptexsockaddrs(0),m_lpfndisconnectex(0){}
		iocp_coderorz(const iocp_coderorz& ref);
		iocp_coderorz& operator=(const iocp_coderorz& ref);
	public:
		virtual ~iocp_coderorz()
		{
			cb_closesock(m_ListenSock);
			CloseHandle(m_hiocp);
			WSACleanup();
		}
		static iocp_coderorz& instance(void){static iocp_coderorz ref;return ref;}
	public:
		static unsigned int __stdcall iocp_proc(void* p)
		{
			iocp_coderorz* pIocpServer = (iocp_coderorz*)p; cb_thread_fd hIocp = pIocpServer->m_hiocp;
			PSOCK_CONTEXT pSockC(0); OVERLAPPED* pIocp_OL(0); DWORD dwBytes(0);
			while(1)
			{
				if(GetQueuedCompletionStatus(hIocp, &dwBytes, (PULONG_PTR)&pSockC, &pIocp_OL, -1))
				{
					if(dwBytes > 0)
					{
						pIocpServer->HandleIOCP(pSockC, CONTAINING_RECORD(pIocp_OL, IO_CONTEXT, _OL_), dwBytes);
						continue;
					}
					if(dwBytes == 0)
					{
						if(!pSockC)
						{
							pIocpServer->DoAccept(CONTAINING_RECORD(pIocp_OL, LISTEN_IO_CONTEXT, _OL_));
							continue;
						}
						if(pSockC){
							pIocpServer->DelSockC(pSockC);
						}
						PIO_CONTEXT pIOC = CONTAINING_RECORD(pIocp_OL, IO_CONTEXT, _OL_);
						if(pIOC){
							pIocpServer->DelIOC(pIOC);
						}
					}
					continue;
				}
				if(pSockC){
					pIocpServer->DelSockC(pSockC);
				}
				if(pIocp_OL)
				{
					PIO_CONTEXT pIOC = CONTAINING_RECORD(pIocp_OL, IO_CONTEXT, _OL_);
					if(pIOC){
						pIocpServer->DelIOC(pIOC);
					}
				}
			}
			//exit
			return 0;
		}
		virtual int timeout_callback(void* p)
		{
			//Node 被锁,不可能被其它的线程从 Node 上 Del 掉
			PSOCK_CONTEXT pSockC = (PSOCK_CONTEXT)p;
			if(pSockC->_SockC_Socket_ != INVALID_SOCKET)
			{
				LINGER lingerStruct; lingerStruct.l_onoff = 1; lingerStruct.l_linger = 0;
				setsockopt(pSockC->_SockC_Socket_, SOL_SOCKET, SO_LINGER, (char*)&lingerStruct, sizeof(lingerStruct));
				if(pSockC->GetIOCount())
				{
					CancelIo((HANDLE)pSockC->_SockC_Socket_);
				}
				cb_closesock(pSockC->_SockC_Socket_);
			}
			return 0;
		}
	public:
		void start(void)
		{
			WSADATA wsaData; ::WSAStartup(MAKEWORD(2, 2), &wsaData);

			m_hiocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
			m_ListenSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			CreateIoCompletionPort((HANDLE)m_ListenSock, m_hiocp, (DWORD)0, 0);

			DWORD dwBytes(0);
			GUID disconnectex = WSAID_DISCONNECTEX;
			WSAIoctl(m_ListenSock, SIO_GET_EXTENSION_FUNCTION_POINTER, 
				&disconnectex, sizeof(disconnectex), &m_lpfndisconnectex, sizeof(m_lpfndisconnectex), &dwBytes, NULL, NULL);
			GUID acceptex = WSAID_ACCEPTEX;
			WSAIoctl(m_ListenSock, SIO_GET_EXTENSION_FUNCTION_POINTER, 
				&acceptex, sizeof(acceptex), &m_lpfnacceptex, sizeof(m_lpfnacceptex), &dwBytes, NULL, NULL);
			GUID acceptexsockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
			WSAIoctl(m_ListenSock, SIO_GET_EXTENSION_FUNCTION_POINTER, 
				&acceptexsockaddrs, sizeof(acceptexsockaddrs), &m_lpfngetacceptexsockaddrs, sizeof(m_lpfngetacceptexsockaddrs), &dwBytes, NULL, NULL);

			struct sockaddr_in saddr	= {0};
			saddr.sin_family			= AF_INET;
			saddr.sin_addr.s_addr		= inet_addr("127.0.0.1");//inet_addr("172.16.50.30");
			saddr.sin_port				= htons(12345);

			bind(m_ListenSock, (struct sockaddr*)&saddr, sizeof(saddr));

			listen(m_ListenSock, SOMAXCONN);

			SYSTEM_INFO sysinfo;
			::GetSystemInfo(&sysinfo);//sysInfo.dwPageSize
			unsigned long ulcount(sysinfo.dwNumberOfProcessors * 2);
			for(unsigned long i = 0; i < ulcount; ++i)
			//while(--ulcount >= 0)
			{
				cb_thread_fd tfd;
				cb_thread_create(tfd, iocp_proc, this);
				cb_thread_fail(tfd);
				cb_thread_close_fd(tfd);
			}

			m_TimeOutProcRef.start(this);
			m_HeartBeatProcRef.start(this, 30000);

			for(int i = 0; i < 10; ++i)
			//while(ulcount++ < 10)
			{
				PostListenIO();
			}
		}
	public:
		PSOCK_CONTEXT GetSockC(void)
		{
			PSOCK_CONTEXT pSockC = m_SockC_Pool.newobj();
			if(!pSockC)
			{
				int isize = sizeof(SOCK_CONTEXT) + __mem_pool_offset__;
				pSockC = (PSOCK_CONTEXT)new char[isize];
				if(!pSockC){
					return 0;
				}
				memset(pSockC, 0, isize);
				pSockC = (PSOCK_CONTEXT)((char*)pSockC + __mem_pool_offset__);
			}
			else{
				memset(pSockC, 0, sizeof(SOCK_CONTEXT));
			}
			char buf[2048] = {0};
			sprintf_s(buf, 2048, "GetSockC.......\n");
			cb_log.writelog(buf);
			return pSockC;
		}

		int DelSockC(PSOCK_CONTEXT pSockC)
		{
			int IOCount = pSockC->DelIOCount();
			if(IOCount == 0)
			{
				cb_closesock(pSockC->_SockC_Socket_);
				m_SockC_Pool.delobj(pSockC);
				char buf[2048] = {0};
				sprintf_s(buf, 2048, "DelSockC.......\n");
				cb_log.writelog(buf);
			}
			else{
				char buf[2048] = {0};
				sprintf_s(buf, 2048, "DelSockC IOCount Error [%d]\n", IOCount);
				cb_log.writelog(buf);
			}
			return IOCount;
		}

		PIO_CONTEXT GetIOC(IO_TYPE _IO_Type)
		{
			PIO_CONTEXT pIOC = m_IOC_Pool.newobj();
			if(!pIOC)
			{
				int isize = sizeof(IO_CONTEXT) + __mem_pool_offset__;
				pIOC = (PIO_CONTEXT)new char[isize];
				if(!pIOC){
					return 0;
				}
				memset(pIOC, 0, isize);
				pIOC = (PIO_CONTEXT)((char*)pIOC + __mem_pool_offset__);
			}
			else{
				memset(pIOC, 0, sizeof(IO_CONTEXT));
			}
			pIOC->_IOBuf_.buf = (char*)m_MemPool.mcb_pool_new(4096);
			if(!pIOC->_IOBuf_.buf)
			{
				int isize = 4096 + __mem_pool_offset__;
				pIOC->_IOBuf_.buf = new char[isize];
				if(!pIOC->_IOBuf_.buf)
				{
					m_IOC_Pool.delobj(pIOC);
					return 0;
				}
				memset(pIOC->_IOBuf_.buf, 0, isize);
				pIOC->_IOBuf_.buf += __mem_pool_offset__;
			}
			else{
				memset(pIOC->_IOBuf_.buf, 0, 4096);
			}
			pIOC->_IO_Type_ = _IO_Type;
			pIOC->_IOBuf_.len = 4096;
			char buf[2048] = {0};
			sprintf_s(buf, 2048, "GetIOC.......\n");
			cb_log.writelog(buf);
			return pIOC;
		}

		void DelIOC(PIO_CONTEXT pIOC)
		{
			if(pIOC)
			{
				if(pIOC->_IOBuf_.buf)
				{
					m_MemPool.mcb_pool_del(pIOC->_IOBuf_.buf);
					pIOC->_IOBuf_.buf = 0;
				}
				m_IOC_Pool.delobj(pIOC);
				char buf[2048] = {0};
				sprintf_s(buf, 2048, "DelIOC.......\n");
				cb_log.writelog(buf);
			}
		}

		char* GetMemory(DWORD dwBytes)
		{
			char* pData = (char*)m_MemPool.mcb_pool_new(dwBytes);
			if(!pData)
			{
				int size = dwBytes + __mem_pool_offset__;
				pData = new char[size];
				if(!pData)
					return 0;
				memset(pData, 0, size);
				return (pData + __mem_pool_offset__);
			}
			memset(pData, 0, dwBytes);
			return pData;
		}

		void DelMemory(void* pDel)
		{
			if(pDel)
			{
				m_MemPool.mcb_pool_del(pDel);
			}
		}

		//????
		int ReuseSocket(SOCKET sock, LPOVERLAPPED pOL = 0)
		{
			if(!pOL){
				cb_closesock(sock);
				return 0;
			}
			struct linger so_linger;
			so_linger.l_onoff = 1; so_linger.l_linger = 0;
			int iRet = setsockopt(sock, SOL_SOCKET, SO_DONTLINGER, (const char*)&so_linger, sizeof(so_linger));
			if(iRet)
			{
				if(m_lpfndisconnectex(sock, pOL/*不为空,异步可能返回error_io_pending,属于正常操作*/, TF_REUSE_SOCKET, 0))
				{
					// 重用 SOCKET
					cb_closesock(sock);

					return 0;
				}
				else{
					printf("555555555555555555555555555[%d][%d]\n", iRet, cb_errno);
					return -1;
				}
			}
			else{
				printf("666666666666666666666666666[%d][%d]\n", iRet, cb_errno);
				return -1;
			}
		}
		
		void DoAccept(PLISTEN_IO_CONTEXT pListenIOC)
		{
			SOCKADDR_IN* pServerSockAddr(0); SOCKADDR_IN* pClientSockAddr(0);
			INT nServerAddrLen(0); INT nClentAddrLen(0);
			DWORD nSockAddrLen(sizeof(SOCKADDR_IN) + 16);
			m_lpfngetacceptexsockaddrs(pListenIOC->_ListenIOBuf_.buf, 0, nSockAddrLen, nSockAddrLen, 
				(LPSOCKADDR*)&pServerSockAddr, &nServerAddrLen, (LPSOCKADDR*)&pClientSockAddr, &nClentAddrLen);
			
			//检测IP是否合法,或者过滤特定IP
			//inet_ntoa(pClientSockAddr->sin_addr);
			
			PSOCK_CONTEXT pSockC = GetSockC();
			if(pSockC)
			{
				memcpy(&pSockC->_SockC_ClientAddr_, pClientSockAddr, sizeof(SOCKADDR_IN));
				pSockC->_SockC_Socket_ = pListenIOC->_Listen_IO_Sock_; pListenIOC->_Listen_IO_Sock_ = INVALID_SOCKET;

				if(CreateIoCompletionPort((HANDLE)pSockC->_SockC_Socket_, m_hiocp, (DWORD)pSockC, 0))
				{
					PIO_CONTEXT pIOC = GetIOC(IO_TYPE_DOFR);
					if(pIOC)
					{
						DWORD dwFlags(0), dwBytes(0);

						m_TimeOutProcRef.add(pSockC);

						pSockC->AddIOCount();

						if(SOCKET_ERROR != WSARecv(pSockC->_SockC_Socket_, &pIOC->_IOBuf_, 1, &dwBytes, &dwFlags, &pIOC->_OL_, NULL) 
							|| (WSA_IO_PENDING == cb_errno))
						{
							PostListenIO(pListenIOC);
							return ;
						}

						pSockC->DelIOCount();

						int iret = m_TimeOutProcRef.del(pSockC);
						if(iret != 0)
						{
							char buf[2048] = {0};
							sprintf_s(buf, 2048, "DoAccept [m_TimeOutProcRef.del] [%d]\n", iret);
							cb_log.writelog(buf);
						}

						DelIOC(pIOC);
						char buf[2048] = {0};
						sprintf_s(buf, 2048, "DoAccept WSARecv(%d)\n", cb_errno);
						cb_log.writelog(buf);
					}
					else
					{
						cb_log.writelog("DoAccept GetIOC(0)\n");
					}
				}
				else
				{
					char buf[2048] = {0};
					sprintf_s(buf, 2048, "DoAccept CreateIoCompletionPort(false)[%d]\n", cb_errno);
					cb_log.writelog(buf);
				}
				cb_closesock(pSockC->_SockC_Socket_);
				m_SockC_Pool.delobj(pSockC);
			}
			else
			{
				cb_closesock(pListenIOC->_Listen_IO_Sock_);
				cb_log.writelog("DoAccept GetSOCKC(0)\n");
			}
			
			PostListenIO(pListenIOC);
			return ;
		}

		int PostListenIO(PLISTEN_IO_CONTEXT pListenIOC = 0)
		{
			if(!pListenIOC)
			{
				if(!(pListenIOC = m_ListenIOC_Pool.newobj()))
				{
					cb_log.writelog("PostListenIO new pListenIOC(0)\n");
					return -1;
				}
				pListenIOC->_ListenIOBuf_.buf = (char*)m_MemPool.mcb_pool_new(4096);
				if(!pListenIOC->_ListenIOBuf_.buf)
				{
					m_ListenIOC_Pool.delobj(pListenIOC);
					pListenIOC = 0;
					cb_log.writelog("PostListenIO new pListenIOC[buf](0)\n");
					return -2;
				}
				pListenIOC->_ListenIOBuf_.len = 4096;
				memset(pListenIOC->_ListenIOBuf_.buf, 0, pListenIOC->_ListenIOBuf_.len);
			}
			pListenIOC->_Listen_IO_Sock_ = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
			if(pListenIOC->_Listen_IO_Sock_ == INVALID_SOCKET)
			{
				if(pListenIOC->_ListenIOBuf_.buf)
				{
					m_MemPool.mcb_pool_del(pListenIOC->_ListenIOBuf_.buf);
					pListenIOC->_ListenIOBuf_.buf = 0;
				}
				pListenIOC->_ListenIOBuf_.len = 0;
				m_ListenIOC_Pool.delobj(pListenIOC);
				cb_log.writelog("PostListenIO WSASocket(INVALID_SOCKET)\n");
				return -3;
			}
			
			DWORD dwBytes(sizeof(sockaddr_in) + 16);
			if(!m_lpfnacceptex(m_ListenSock, pListenIOC->_Listen_IO_Sock_, pListenIOC->_ListenIOBuf_.buf, 0, 
				dwBytes, dwBytes, &dwBytes, &pListenIOC->_OL_) && WSA_IO_PENDING != cb_errno)
			{
				if(pListenIOC->_ListenIOBuf_.buf)
				{
					m_MemPool.mcb_pool_del(pListenIOC->_ListenIOBuf_.buf);
					pListenIOC->_ListenIOBuf_.buf = 0;
				}
				pListenIOC->_ListenIOBuf_.len = 0;

				cb_closesock(pListenIOC->_Listen_IO_Sock_);
				cb_log.writelog("PostListenIO false\n");
				return -4;
			}
			return 0;
		}

		void HandleIOCP(PSOCK_CONTEXT pSockC, PIO_CONTEXT pIOC, DWORD dwBytes)
		{
			switch(pIOC->_IO_Type_)
			{
			case IO_TYPE_DOFR:
				HandleFirstRecv(pSockC, pIOC, dwBytes);
				break;
			case IO_TYPE_RECV:
				HandleRecv(pSockC, pIOC, dwBytes);
				break;
			case IO_TYPE_POST:

				break;
			case IO_TYPE_NULL:
				break;
			}
		}
	private:
		int HandleFirstRecv(PSOCK_CONTEXT pSockC, PIO_CONTEXT pIOC, DWORD dwBytes)
		{
			//同一个SOCKET只投递了一个IO,不可能被多个线程调用
			int iRet = 0; char* pData = 0;
			PDATA_HEAD pDataHead = (PDATA_HEAD)pIOC->_IOBuf_.buf;
			if(dwBytes <= 0 || dwBytes < sizeof(DATA_HEAD) || !pDataHead || pDataHead->_DataLength_ <= 0 || 
				(iRet = m_TimeOutProcRef.del(pSockC)) != 0 || !(pData = GetMemory(pDataHead->_DataLength_ + 1)))
			{
				DelIOC(pIOC);
				int DelSockCRet = DelSockC(pSockC);
				if(DelSockCRet)
				{
					char buf[1024] = {0};
					sprintf_s(buf, 1023, "HandleFirstRecv Error [DelSockC] [%d]\n", DelSockCRet);
					cb_log.writelog(buf);
				}
				char buf[1024] = {0};
				sprintf_s(buf, 1023, "HandleFirstRecv Error [%d][%d][%d]\n", iRet, dwBytes, pDataHead ? pDataHead->_DataLength_ : 0);
				cb_log.writelog(buf);
				return -1;
			}
			else{
				if(dwBytes < pDataHead->_DataLength_)
				{

				}
				else if(dwBytes == pDataHead->_DataLength_){

				}
				else{

				}
				memcpy(pData, pIOC->_IOBuf_.buf, dwBytes);
				if(pData)
				{
					DelMemory(pData);
				}
				//------------------------>
				char buf[8192 + 1024] = {0};
				sprintf_s(buf, 8192 + 1024, "%s %d 发送信息 : %s [%d]\n", 
					inet_ntoa(pSockC->_SockC_ClientAddr_.sin_addr), ntohs(pSockC->_SockC_ClientAddr_.sin_port), pData, dwBytes);
				cb_log.writelog(buf);
				printf(buf);

				DelMemory(pData);
				//<------------------------

				m_HeartBeatProcRef.add(pSockC);
				pIOC->_IO_Type_ = IO_TYPE_RECV;
				return PostRecv(pSockC, pIOC);
			}
		}
		
		int HandleRecv(PSOCK_CONTEXT pSockC, PIO_CONTEXT pIOC, DWORD dwBytes)
		{
			char* pData = 0; int iRet = 0;
			if(dwBytes <= 0 || (iRet = m_HeartBeatProcRef.exchange_hb(pSockC)) != 0 || !(pData = GetMemory(dwBytes + 1)))
			{
				if(pData)
				{
					DelMemory(pData);
				}
				m_HeartBeatProcRef.del(pSockC);

				DelIOC(pIOC);
				int DelSockCRet = DelSockC(pSockC);
				if(DelSockCRet)
				{
					char buf[2048] = {0};
					sprintf_s(buf, 2048, "HandleRecv Error [DelSockC] [%d]\n", DelSockCRet);
					cb_log.writelog(buf);
				}
				char buf[2048] = {0};
				sprintf_s(buf, 2048, "HandleRecv Error [%d][%d]\n", iRet, dwBytes);
				cb_log.writelog(buf);
				return -1;
			}
			else{
				memcpy(pData, pIOC->_IOBuf_.buf, dwBytes);

				//------------------------>
				char buf[8192 + 1024] = {0};
				sprintf_s(buf, 8192 + 1024, "%s %d 发送信息 : %s [%d]\n", 
					inet_ntoa(pSockC->_SockC_ClientAddr_.sin_addr), ntohs(pSockC->_SockC_ClientAddr_.sin_port), pData, dwBytes);
				cb_log.writelog(buf);
				printf(buf);

				DelMemory(pData);
				//<------------------------

				return PostRecv(pSockC, pIOC);
			}
		}

		int PostRecv(PSOCK_CONTEXT pSockC, PIO_CONTEXT pIOC)
		{
			DWORD dwBytes(0), dwFlags(0);
			if(SOCKET_ERROR == WSARecv(pSockC->_SockC_Socket_, &pIOC->_IOBuf_, 1, &dwBytes, &dwFlags, &pIOC->_OL_, NULL) 
				&& (WSA_IO_PENDING != cb_errno))
			{
				m_HeartBeatProcRef.del(pSockC);

				DelIOC(pIOC);
				int DelSockCRet = DelSockC(pSockC);
				if(DelSockCRet)
				{
					char buf[2048] = {0};
					sprintf_s(buf, 2048, "PostRecv Error [DelSockC] [%d]\n", DelSockCRet);
					cb_log.writelog(buf);
				}
				char buf[2048] = {0};
				sprintf_s(buf, 2048, "PostRecv Error [WSARecv] [%d]\n", cb_errno);
				cb_log.writelog(buf);
				return -1;
			}
			return 0;
		}

		int PostSend(PSOCK_CONTEXT pSockC, PIO_CONTEXT pIOC)
		{
			DWORD dwBytes(0);
			if(SOCKET_ERROR == WSASend(pSockC->_SockC_Socket_, &pIOC->_IOBuf_, 1, &dwBytes, 0, &pIOC->_OL_, NULL) 
				&& (WSA_IO_PENDING != cb_errno))
			{
				m_HeartBeatProcRef.del(pSockC);

				DelIOC(pIOC);
				int DelSockCRet = DelSockC(pSockC);
				if(DelSockCRet)
				{
					char buf[2048] = {0};
					sprintf_s(buf, 2048, "PostSend[DelSockC] [%d]\n", DelSockCRet);
					cb_log.writelog(buf);
				}
				char buf[2048] = {0};
				sprintf_s(buf, 2048, "PostSend [WSASend] [%d]\n", cb_errno);
				cb_log.writelog(buf);
				return -1;
			}
			return 0;
		}
	public:
		cb_thread_fd				m_hiocp;
	private:
		cb_socket					m_ListenSock;
		LPFN_ACCEPTEX				m_lpfnacceptex;
		LPFN_GETACCEPTEXSOCKADDRS	m_lpfngetacceptexsockaddrs;
		LPFN_DISCONNECTEX			m_lpfndisconnectex;
		cb_space_memorypool::obj_pool<LISTEN_IO_CONTEXT> m_ListenIOC_Pool;
		cb_space_memorypool::obj_pool<SOCK_CONTEXT> m_SockC_Pool;
		cb_space_memorypool::obj_pool<IO_CONTEXT> m_IOC_Pool;
		cb_space_memorypool::mem_pool m_MemPool;
	private:
		__cb_class__				m_TimeOutProcRef;
		__cb_class__				m_HeartBeatProcRef;
	};
#endif
	class epoll_coderorz{};
};
//https://coolshell.cn/articles/11564.html

//https://www.cnblogs.com/lidabo/p/6042704.html

//https://www.cnblogs.com/eeexu123/p/5275783.html

#endif
