#ifndef iocptcpclient_h__
#define iocptcpclient_h__
#include <Winsock2.h>
#include <Windows.h>
#include <MSTCPiP.h>
#include <MSWSock.h>
#include <process.h>
#pragma comment(lib, "Ws2_32.lib")
#include <stdio.h>
#include <stdlib.h>
#include <string>

enum opCode {OP_CONNECT, OP_DISCONNECT, OP_RECV, OP_SEND};

class iocp_client
{
	struct PER_IO_CONTEXT
	{
		OVERLAPPED _overlapped;
		opCode _op;
		SOCKET sock;
		int datalen;
		WSABUF _wsaBuf;

		PER_IO_CONTEXT(opCode op):_op(op) {
			::ZeroMemory(&_overlapped, sizeof(_overlapped));
		}
	};
	struct tparams
	{
		iocp_client* pthis;
		int type;
		int id;
	};
	struct head
	{
		char _4_flag[4];
		char _4_length[4];
		char _4_btype[4];
		char _4_ex1[4];
		char _4_ex2[4];
		char _4_ex3[4];
		char _4_ex4[4];
		char _4_ex5[4];
	};
public:
	iocp_client(){}
	virtual ~iocp_client(){}
public:
	void start(const std::string& ip, int port)
	{
		WSAData wsaData; ::WSAStartup(MAKEWORD(2,2), &wsaData);

		m_hiocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);

// 		m_sock = ::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
// 
// 		struct sockaddr_in addr;
// 		::ZeroMemory(&addr, sizeof(addr));
// 		addr.sin_family = AF_INET;
// 		addr.sin_addr.s_addr = INADDR_ANY;
// 		addr.sin_port = 0;
// 
// 		::bind(m_sock, (sockaddr *)&addr, sizeof(addr));
// 
// 		::CreateIoCompletionPort((HANDLE)m_sock, m_hiocp, 0, 0);
// 
// 		DWORD dwBytes(0);
// 		GUID AcceptEX = WSAID_CONNECTEX;
// 		WSAIoctl(m_sock, SIO_GET_EXTENSION_FUNCTION_POINTER, 
// 			&AcceptEX, sizeof(AcceptEX), &m_lpConnectEx, sizeof(m_lpConnectEx), &dwBytes, NULL, NULL);

		::ZeroMemory(&m_remote_addr_, sizeof(m_remote_addr_));
		m_remote_addr_.sin_addr.s_addr = inet_addr(ip.c_str());
		m_remote_addr_.sin_port = htons(port);
		m_remote_addr_.sin_family = AF_INET;

		CloseHandle((HANDLE)_beginthreadex(0, 0, iocp_proc, this, 0, 0));

		int type = 0;
		for(int i = 0; i < 1; ++i)
		{
			tparams* ptparams = new tparams;
			ptparams->id = i;
			ptparams->type = type;
			ptparams->pthis = this;
			DWORD tid;
			HANDLE thd = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)runConnect, ptparams, 0, &tid);
			::CloseHandle(thd);
		}
		type = 1;
		for(int i = 0; i < 0; ++i)
		{
			tparams* ptparams = new tparams;
			ptparams->id = i;
			ptparams->type = type;
			ptparams->pthis = this;
			DWORD tid;
			HANDLE thd = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)run1, ptparams, 0, &tid);
			::CloseHandle(thd);
		}
		type = 2;
		for(int i = 0; i < 0; ++i)
		{
			tparams* ptparams = new tparams;
			ptparams->id = i;
			ptparams->type = type;
			ptparams->pthis = this;
			DWORD tid;
			HANDLE thd = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)run2, ptparams, 0, &tid);
			::CloseHandle(thd);
		}
	}
public:
	static unsigned int __stdcall iocp_proc(void* p)
	{
		iocp_client* pthis = (iocp_client*)p;
		HANDLE _hIOCP = pthis->m_hiocp;
		DWORD bytesTransferred;
		ULONG_PTR pSockC;
		LPOVERLAPPED pOverlapped;

		while (true)
		{
			BOOL ok = ::GetQueuedCompletionStatus(_hIOCP, &bytesTransferred, &pSockC, &pOverlapped, INFINITE);
			if (!ok) { // ERROR EXIT
				fprintf(stderr, "GetQueuedCompletionStatus failed error code = %d", ::GetLastError());
				break;
			}
			if (pSockC == INVALID_SOCKET) { // notified EXIT
				break;
			}

			/////////////////////////////////////////////////////////

			PER_IO_CONTEXT *context = CONTAINING_RECORD(pOverlapped, PER_IO_CONTEXT, _overlapped);
			if (bytesTransferred == 0 && 
				( context->_op == OP_RECV)) {// disconnect EXIT
					//break; 
			}

			switch (context->_op)
			{
			case OP_CONNECT:
				/*
				int ret = ::setsockopt(
				sock_,
				SOL_SOCKET,
				SO_UPDATE_CONNECT_CONTEXT,
				0,
				0
				);
				return (ret != SOCKET_ERROR);
				*/
				pthis->IOCPConnect(context);
				break;
			case OP_DISCONNECT:

				break;
			case OP_RECV:
				pthis->IOCPRecv(context, bytesTransferred);
				break;
			case OP_SEND:
				pthis->IOCPSent(context, bytesTransferred);
				break;
			default:
				fprintf(stderr, "NOT FOUND OPCODE");
				break;
			}
		}
		return 0;
	}
	static DWORD WINAPI runConnect(LPVOID param)
	{
		tparams* ptparams = (tparams*)param;
		if(!ptparams) return 0;

		SOCKET* s(0);
		ptparams->pthis->ConnectToServer();

		delete ptparams, ptparams = 0;
		return 0;
	}
	static DWORD WINAPI run1(LPVOID param)
	{
		tparams* ptparams = (tparams*)param;
		if(!ptparams) return 0;

		SOCKET* s(0);
		//ptparams->pthis->ConnectToServer(s, ptparams->pthis);


		delete ptparams, ptparams = 0;
		return 0;
	}
	static DWORD WINAPI run2(LPVOID param)
	{
		tparams* ptparams = (tparams*)param;
		if(!ptparams) return 0;

		SOCKET* s(0);
		//ptparams->pthis->ConnectToServer(s, ptparams->pthis);


		delete ptparams, ptparams = 0;
		return 0;
	}
private:
	int ConnectToServer(void)
	{
		SOCKET m_sock = ::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

		struct sockaddr_in addr;
		::ZeroMemory(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = 0;

		::bind(m_sock, (sockaddr *)&addr, sizeof(addr));

		::CreateIoCompletionPort((HANDLE)m_sock, m_hiocp, 0, 0);

		DWORD dwBytes(0);
		GUID AcceptEX = WSAID_CONNECTEX;
		WSAIoctl(m_sock, SIO_GET_EXTENSION_FUNCTION_POINTER, 
			&AcceptEX, sizeof(AcceptEX), &m_lpConnectEx, sizeof(m_lpConnectEx), &dwBytes, NULL, NULL);
		PER_IO_CONTEXT* pConnContext = new PER_IO_CONTEXT(OP_CONNECT);
		pConnContext->sock = m_sock;
		m_lpConnectEx(m_sock, (SOCKADDR*)&m_remote_addr_, sizeof(m_remote_addr_), 0, 0, 0, (LPOVERLAPPED)&(pConnContext->_overlapped));

		return 0;
	}
	int IOCPConnect(PER_IO_CONTEXT* pIOC)
	{
		char head[32] = {0};

		char* pdata = new char[8192];
		memset(pdata, 0, 8192);
		int len = 0; int j = 0;
		while(len < 8100)
		{
			len += sizeof(head);
			int bodylen = 0;
			for(int i = 0; i < 3; ++i)
			{
				char temp[128] = {0};
				sprintf_s(temp, sizeof(temp), "[%04d][%d]Hello World!!!!!!!!!!!!", j, i+1);
				bodylen += strlen(temp);
				memcpy(&pdata[len], temp, strlen(temp));
				len += strlen(temp);
			}
			//printf("%d:%d:%x\n", len - bodylen - sizeof(head), bodylen, &pdata[len - bodylen - sizeof(head)]);

			SetServerFlag(head);
			SetBusinessType(head, 0);
			SetDataLength(head, bodylen);
			memcpy(&pdata[len - bodylen - sizeof(head)], head, sizeof(head));

			++j;
		}
		

		pIOC->_wsaBuf.buf = pdata;
		pIOC->datalen = len;
		pIOC->_wsaBuf.len = /*datalen + sizeof(head)*/sizeof(head);
		pIOC->_op = OP_SEND;
		DWORD dwBytes(0);
		if(SOCKET_ERROR == WSASend(pIOC->sock, &pIOC->_wsaBuf, 1, &dwBytes, 0, &pIOC->_overlapped, NULL) 
			&& (WSA_IO_PENDING != WSAGetLastError()))
		{
			printf("WSASend err[%d]", WSAGetLastError());
		}

		PER_IO_CONTEXT* pIOC_RECV = new PER_IO_CONTEXT(OP_RECV);
		pIOC_RECV->sock = pIOC->sock;
		pIOC_RECV->_wsaBuf.buf = new char[8192];
		pIOC_RECV->_wsaBuf.len = 8192;
		memset(pIOC_RECV->_wsaBuf.buf, 0, pIOC_RECV->_wsaBuf.len);
		DWORD dwBytesRecv(0), dwFlags(0);
		if(SOCKET_ERROR == WSARecv(pIOC_RECV->sock, &pIOC_RECV->_wsaBuf, 1, &dwBytesRecv, &dwFlags, &pIOC_RECV->_overlapped, NULL) 
			&& (WSA_IO_PENDING != WSAGetLastError()))
		{
			printf("WSARecv err[%d]", WSAGetLastError());
		}
		return 0;
	}
	int IOCPSent(PER_IO_CONTEXT* pIOC, DWORD dwBytes)
	{
		if((int)dwBytes < pIOC->datalen)
		{
			pIOC->datalen -= dwBytes;
			pIOC->_wsaBuf.len = 32;
			//printf("%d\n", dwBytes);
			memmove(pIOC->_wsaBuf.buf, &(pIOC->_wsaBuf.buf[dwBytes]), pIOC->datalen);
			DWORD dwBytes(0);
			if(SOCKET_ERROR == WSASend(pIOC->sock, &pIOC->_wsaBuf, 1, &dwBytes, 0, &pIOC->_overlapped, NULL) 
				&& (WSA_IO_PENDING != WSAGetLastError()))
			{
				printf("WSASend err[%d]", WSAGetLastError());
			}
		}
		return 0;
	}
	int IOCPRecv(PER_IO_CONTEXT* pIOC, DWORD dwBytes)
	{
		if(dwBytes > 32)
		{
			std::string chbo(pIOC->_wsaBuf.buf, 4);
			printf(chbo.c_str()); printf("\n");
			int nbodylen = GetBodyLength(pIOC->_wsaBuf.buf);
			printf("%d:%d\n", nbodylen, dwBytes);
			if(nbodylen == dwBytes - 32)
			{
				printf(&pIOC->_wsaBuf.buf[32]);
				memset(pIOC->_wsaBuf.buf, 0, pIOC->_wsaBuf.len);
				DWORD dwBytesRecv(0), dwFlags(0);
				if(SOCKET_ERROR == WSARecv(pIOC->sock, &pIOC->_wsaBuf, 1, &dwBytesRecv, &dwFlags, &pIOC->_overlapped, NULL) 
					&& (WSA_IO_PENDING != WSAGetLastError()))
				{
					printf("IOCPRecv WSARecv err[%d]", WSAGetLastError());
				}
			}
			else{
				printf("......\n");
			}
		}
		return 0;
	}
private:
	void SetServerFlag(char* pHead)
	{
		memcpy(pHead, "chbo", 4);
	}
	void SetDataLength(char* pHead, unsigned int nLength)
	{
		*(unsigned int*)(&pHead[4]) = nLength;
	}
	void SetBusinessType(char* pHead, unsigned int nType)
	{
		*(unsigned int*)(&pHead[8]) = nType;
	}
	DWORD GetBodyLength(void* p){return *((DWORD*)&((char*)(p))[4]);}
	DWORD GetBusinessType(void* p){return *((int*)(&((char*)p)[8]));}
private:
	HANDLE m_hiocp;
	LPFN_CONNECTEX m_lpConnectEx;
	struct sockaddr_in m_remote_addr_;
};

#endif // iocptcpclient_h__