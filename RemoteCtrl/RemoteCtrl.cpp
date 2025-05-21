// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include "EdoyunTool.h"
#include <conio.h>
#include "CEdoyunQueue.h"
#include <MSWSock.h>
#include "EdoyunServer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象
// define  INVOKE_PATH _T("C::\\Windows\\SysWOW64\\RemoteCtrl.exe")
#define  INVOKE_PATH _T("C:\\Users\\difeng\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")

CWinApp theApp;
using namespace std;

//业务和通用
bool ChooseAutoInvoke(const CString& strPath) {
    TCHAR wcsSystem[MAX_PATH] = _T("");
    //CString strPath = CString(_T("C::\\Windows\\SysWOW64\\RemoteCtrl.exe"));
    if (PathFileExists(strPath)) {
        return true;
    }

    CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
    CString strInfo = _T("该程序只允许用于合法的用途！\n");
    strInfo += _T("继续运行该程序，将使得这台机器处于被监控状态！\n");
    strInfo += _T("如果你不希望这样，请按“取消”按钮，退出程序。\n");
    strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
    strInfo += _T("按下“否”按钮，程序只运行一次，不会在系统内留下任何东西！\n");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if(ret == IDYES){
        //WriteRegisterTable(strPath);
        if (!CEdoyunTool::WriteStartupDir(strPath)) 
        {
            MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
            return false;
        }
    }
    else if (ret == IDCANCEL) {
        return false;
    }
    return true;
}

void iocp();
void udp_server();
void udp_client(bool ishost = true);

int main(int argc, char* argv[])
{
    if (!CEdoyunTool::Init()) return 1;  //判断初始化是否成功
    //initsock();
    //if (argc == 1) { //服务器代码
    //    char wstrDir[MAX_PATH];
    //    GetCurrentDirectoryA(MAX_PATH, wstrDir);
    //    STARTUPINFOA si;
    //    PROCESS_INFORMATION pi;
    //    memset(&si, 0, sizeof(si));
    //    memset(&pi, 0, sizeof(pi));
    //    string strCmd = argv[0];
    //    strCmd += " 1";
    //    BOOL bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
    //    if (bRet) {
    //        CloseHandle(pi.hThread);
    //        CloseHandle(pi.hProcess);
    //        TRACE("进程ID:%d\r\n", pi.dwProcessId);
    //        TRACE("线程ID:%d\r\n", pi.dwThreadId);
    //        strCmd += " 2";
    //        BOOL bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
    //        if (bRet) {
    //            CloseHandle(pi.hThread);
    //            CloseHandle(pi.hProcess);
    //            TRACE("进程ID:%d\r\n", pi.dwProcessId);
    //            TRACE("线程ID:%d\r\n", pi.dwThreadId);
    //            udp_server();//服务器代码
    //        }
    //    }
    //}
    //else if (argc == 2) { //主客户端代码
    //    udp_client();
    //}
    //else {  //从客户端代码
    //    udp_client(false);
    //}
    //clearsock();
    iocp();
     /*
    if (CEdoyunTool::IsAdmin()) {   //判断是够为管理员
        
        if (ChooseAutoInvoke(INVOKE_PATH)) { //判断自启动是否成功
            CCommand cmd;
            int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
            switch (ret) {
            case -1:
                MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
                break;
            case -2:
                MessageBox(NULL, _T("多次无法正常接入用户,结束程序！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                break;
            }
        }
    }
    else {
        if (CEdoyunTool::RunAsAdmin() == false) {
            CEdoyunTool::ShowError();
            return 1;
        }

    }  */
    return 0;
}

class COverlapped {
public:
    OVERLAPPED m_overlapped;
    DWORD m_operator;
    char m_buffer[4096];
    COverlapped() {
        m_operator = 0;
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        memset(m_buffer, 0, sizeof(m_buffer));
    }
};
void iocp()
{
    EdoyunServer server;
    server.StartService();
    getchar();
}

void initsock() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
}

void clearsock() {
    WSACleanup();
}
void udp_server()
{

    printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
    SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("%s(%d):%s ERROR(%d)!!!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
        return;
    }
    std::list<sockaddr_in>lstclients;
    sockaddr_in server, client;
    memset(&server, 0, sizeof(server));
    memset(&client, 0, sizeof(client));
    server.sin_family = AF_INET;
    server.sin_port = htons(20000); //UDP端口尽量设置的大一些 10000以上  
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (-1 == bind(sock, (sockaddr*)&server, sizeof(server))) {
        printf("%s(%d):%s ERROR(%d)!!!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
        closesocket(sock);
        return;
    }
    char buf[4096] = "";
    int len = 0;
    int ret = 0;
    while (!_kbhit()) {
        ret = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&client, &len);
        if (ret > 0) {
            lstclients.push_back(client);
            CEdoyunTool::Dump((BYTE*)buf, ret);
            printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__ ,client.sin_addr.s_addr, client.sin_port);
            ret = sendto(sock, buf, ret, 0, (sockaddr*)&client, len);
            printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
        }
        else {
            printf("%s(%d):%s ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(),ret);
        }
    }
    closesocket(sock);
    printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);

}
void udp_client(bool ishost) 
{
    Sleep(2000);
    sockaddr_in server,client;
    int len = 0;
    server.sin_family = AF_INET;
    server.sin_port = htons(20000); //UDP端口尽量设置的大一些 10000以上  
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("%s(%d):%s ERROR!!!\r\n", __FILE__, __LINE__, __FUNCTION__);
        return;
    }
    if (ishost) { //主客户端代码
        printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
        std::string msg = "hello world!\n";
        int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server));
        printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__,ret);
        if (ret > 0) {
            ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
            printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
            if (ret > 0) {
                printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, client.sin_port);
            }
        }
    }
    else {//从客户端代码
        printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
    }
}
