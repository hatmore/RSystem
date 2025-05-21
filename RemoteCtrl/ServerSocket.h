#pragma once
#include "pch.h"
#include "framework.h"
#include <list>
#include "Packet.h"


typedef void(*SOCKET_CALLBACK)(void* arg,int status, std::list <CPacket>&,CPacket&);

class CServerSocket
{
public:
	static CServerSocket* getInstance() {//静态函数  还是public  在类外面可以调用（语法上面的规定）
		if (m_instance == NULL) {  //静态函数没有this 指针，所以无法直接访问成员变量
			m_instance = new CServerSocket();
		}
		return m_instance;
	}
	bool InitSocket(short port) { //检查初始化有没有成功

		if (m_sock == -1) return false;
		//TODO:校验
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;//监听所有IP
		serv_adr.sin_port = htons(port);
		//绑定
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
			return false;
		}
		//TODO:
		if (listen(m_sock, 1) == -1) {
			return false;
		}
		
		return true;
	}

	int Run(SOCKET_CALLBACK callback, void* arg, short port = 9527) {
		//1 进度的可控性  2  对接的方便性  3 可行性评估，提早暴露风险
		// TODO: socket、bind、listen、accept、read、write、close
		//套接字初始化
		bool ret = InitSocket(port);
		if (ret == false) return -1;
		std::list <CPacket> lstPackets;
		m_callback = callback;
		m_arg = arg;
		int count = 0;
		while (true) {
			if (AcceptClient() == false) {
				if (count >= 3) {
					return -2;
				}
				count++;
			}
			int ret = DealCommand();
			if (ret > 0) {
				m_callback(m_arg, ret, lstPackets,m_packet);
				while (lstPackets.size() > 0) {
					Send(lstPackets.front());
					lstPackets.pop_front();
				}
			}
			CloseClient();
		}
		return 0;
	}
protected:
	bool AcceptClient() { // 接收客户端
		TRACE("enter AcceptClient\r\n");
		sockaddr_in client_adr;
		int cli_sz = sizeof(client_adr);
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		TRACE("m_client = %d\r\n", m_client);
		if (m_client == -1) return false;
		return true;

	}
#define BUFFER_SIZE 4096
	int DealCommand() {
		if (m_client == -1) return -1;
		//char buffer[1024] = "";
		char* buffer = new char[BUFFER_SIZE];
		if (buffer == NULL) {
			TRACE("内存不足!\r\n");
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true) {
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {
				delete[]buffer;
				return -1;
			}
			TRACE("recv %d\r\n", len);
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				delete[]buffer;
				return m_packet.sCmd;
			}
		}
		delete[]buffer;
		return -1;
	}

	bool Send(const char* pData, int nSize) { //不用封装  直接发走  （本项目可能用不到，迁移以后可能会用到）
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack) {
		if (m_client == -1) return false;
		//Dump((BYTE*)pack.Data(), pack.Size());
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}

	void CloseClient() {
		if (m_client != INVALID_SOCKET) {
			closesocket(m_client);
			m_client = INVALID_SOCKET;
		}
	}
private:
	SOCKET_CALLBACK m_callback;
	void* m_arg;
	SOCKET m_client;
	SOCKET m_sock;
	CPacket m_packet;
	CServerSocket& operator=(const CServerSocket& ss) {}
	CServerSocket(const CServerSocket& ss) {
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}

	CServerSocket() {
		m_client = INVALID_SOCKET; // -1
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);//MB_OK | MB_ICONERROR 结合起来表示消息框会显示一个包含“确定”按钮和一个错误图标的消息框
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup();
	}

	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 0), &data) != 0) {
			return FALSE;
		}
		return TRUE;
	}
	static void releaseInstance() {
		if (m_instance != NULL) { //防御性编程
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	static CServerSocket* m_instance;
	class CHelper {
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};

extern CServerSocket server;