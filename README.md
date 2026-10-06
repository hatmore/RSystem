# RSystem（远程控制系统）

C/S 架构的 Windows 远程控制系统：**被控端 RemoteCtrl** 运行在目标机器上，**控制端 RemoteClient** 连接后可以采集设备信息、浏览与传输文件、远程查看屏幕并操作鼠标、锁定/解锁被控机。两端均使用 **MFC** 开发，网络层直接基于 Winsock TCP。

## 目录结构

```
RSystem/
├── RemoteCtrl.sln                 # VS 解决方案（RemoteCtrl + RemoteClient）
├── RemoteCtrl/                    # 被控端（服务端），默认监听 TCP 9527
│   ├── RemoteCtrl.cpp/.h          #   MFC 应用入口（无窗口后台运行）
│   ├── ServerSocket.cpp/.h        #   Winsock 服务端封装：bind / listen / accept / 收发
│   ├── Packet.h                   #   协议包 CPacket 的封装与解析
│   ├── Command.cpp/.h             #   命令分发表（命令号 → 处理函数）
│   ├── EdoyunServer.cpp/.h        #   服务循环，驱动 ServerSocket 与 Command
│   ├── EdoyunThread.cpp/.h        #   线程封装
│   ├── CEdoyunQueue.h             #   线程安全队列（IOCP 实现）
│   ├── EdoyunTool.cpp/.h          #   公共工具（Dump、字符串、系统信息）
│   └── LockDialog.cpp/.h          #   锁屏对话框
└── RemoteClient/                  # 控制端（客户端）
    ├── RemoteClient.cpp/.h        #   MFC 应用入口
    ├── RemoteClientDlg.cpp/.h     #   主对话框：连接、磁盘/目录树、文件操作
    ├── CWatchDialog.cpp/.h        #   远程桌面监视对话框：图传、鼠标事件、锁屏
    ├── StatusDlg.cpp/.h           #   状态对话框
    ├── ClientController.cpp/.h    #   客户端控制器：命令发送、消息分发（MVC 中的 C）
    ├── ClientSocket.cpp/.h        #   Winsock 客户端封装
    └── CEdoyunQueue.cpp           #   线程安全队列
```

## 编译环境

| 项目 | 要求 |
|------|------|
| IDE | Visual Studio 2019 / 2022（平台工具集 v143） |
| 语言 | C/C++，MFC（静态/动态链接均配置） |
| 系统 | Windows 10 及以上，x86 / x64 |
| 网络 | TCP/IP（Winsock2） |

安装 Visual Studio 时需勾选「使用 C++ 的桌面开发」以及「MFC 和 ATL 支持」。

## 构建与运行

1. 用 Visual Studio 打开 `RemoteCtrl.sln`。
2. 选择 `Debug|x64` 或 `Release|x64`，生成解决方案。
3. 先在被控机上运行 **RemoteCtrl.exe**（后台监听 9527 端口），再在控制机上运行 **RemoteClient.exe**，输入被控机 IP 与端口后连接。

> 端口在 `RemoteCtrl/ServerSocket.h` 的 `Run()` 默认参数中修改。

## 通信协议

所有数据以 `CPacket`（`RemoteCtrl/Packet.h`）打包，1 字节对齐：

| 字段 | 类型 | 说明 |
|------|------|------|
| sHead | WORD | 包头，固定 `0xFEFF` |
| nLength | DWORD | 从 `sCmd` 到 `sSum` 的长度（= 数据长度 + 4） |
| sCmd | WORD | 命令号 |
| strData | BYTE[] | 命令数据 |
| sSum | WORD | 数据区逐字节求和校验 |

### 命令列表

| 命令号 | 功能 | 数据 |
|-------:|------|------|
| 1 | 获取驱动器盘符列表 | 无 |
| 2 | 获取目录内容 | 目录路径 |
| 3 | 远程打开/运行文件 | 文件路径 |
| 4 | 下载文件 | 文件路径，分片回传 |
| 5 | 鼠标事件 | `MOUSEEV` 结构（动作、按键、坐标） |
| 6 | 发送屏幕截图 | 无，返回 JPEG 图像 |
| 7 | 锁定被控机 | 无 |
| 8 | 解锁被控机 | 无 |
| 9 | 删除被控机文件 | 文件路径 |
| 1981 | 测试连接 | 无 |

## 设计说明

- 服务端收包流程：`WSAStartup` → `socket` → `bind` → `listen` → `accept` → `recv` / `send`，由 `CServerSocket` 封装，`CCommand` 通过命令号查表分发到具体处理函数。
- 客户端采用 MVC 思路：对话框只负责 UI，`CClientController` 统一发送命令并把结果以窗口消息分发回对应对话框；收发在独立线程中进行，通过 `CEdoyunQueue` 排队。
- 远程桌面监视以轮询方式请求截图（命令 6），鼠标操作实时下发（命令 5）。

## 许可

本项目暂未声明开源许可证。
