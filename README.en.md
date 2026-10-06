[中文](README.md) | **English**

# RSystem (Remote Control System)

A client/server remote-control system for Windows. The **controlled side, RemoteCtrl**, runs on the target machine; the **controller, RemoteClient**, connects to it to collect device information, browse and transfer files, view the remote screen, drive the mouse, and lock or unlock the machine. Both sides are built with **MFC**; networking is plain Winsock TCP.

## Layout

```
RSystem/
├── RemoteCtrl.sln                 # VS solution (RemoteCtrl + RemoteClient)
├── RemoteCtrl/                    # controlled side (server), listens on TCP 9527 by default
│   ├── RemoteCtrl.cpp/.h          #   MFC entry point (runs in the background, no window)
│   ├── ServerSocket.cpp/.h        #   Winsock server: bind / listen / accept / send / recv
│   ├── Packet.h                   #   CPacket protocol packet, build and parse
│   ├── Command.cpp/.h             #   command dispatch table (command id → handler)
│   ├── EdoyunServer.cpp/.h        #   server loop driving ServerSocket and Command
│   ├── EdoyunThread.cpp/.h        #   thread wrapper
│   ├── CEdoyunQueue.h             #   thread-safe queue (IOCP based)
│   ├── EdoyunTool.cpp/.h          #   helpers (dump, strings, system info)
│   └── LockDialog.cpp/.h          #   lock-screen dialog
└── RemoteClient/                  # controller (client)
    ├── RemoteClient.cpp/.h        #   MFC entry point
    ├── RemoteClientDlg.cpp/.h     #   main dialog: connect, drive/directory tree, file ops
    ├── CWatchDialog.cpp/.h        #   remote desktop view: screen stream, mouse events, lock
    ├── StatusDlg.cpp/.h           #   status dialog
    ├── ClientController.cpp/.h    #   controller: sends commands, dispatches results (the C in MVC)
    ├── ClientSocket.cpp/.h        #   Winsock client wrapper
    └── CEdoyunQueue.cpp           #   thread-safe queue
```

## Build environment

| Item | Requirement |
|------|-------------|
| IDE | Visual Studio 2019 / 2022 (platform toolset v143) |
| Language | C/C++, MFC (static and dynamic linking both configured) |
| OS | Windows 10 or later, x86 / x64 |
| Network | TCP/IP (Winsock2) |

Install Visual Studio with "Desktop development with C++" and "MFC and ATL support".

## Build and run

1. Open `RemoteCtrl.sln` in Visual Studio.
2. Select `Debug|x64` or `Release|x64` and build the solution.
3. Run **RemoteCtrl.exe** on the controlled machine first (listens on port 9527 in the background), then run **RemoteClient.exe** on the controller, enter the target IP and port, and connect.

> The port is the default argument of `Run()` in `RemoteCtrl/ServerSocket.h`.

## Protocol

All data is framed as `CPacket` (`RemoteCtrl/Packet.h`), 1-byte packed:

| Field | Type | Description |
|-------|------|-------------|
| sHead | WORD | header, always `0xFEFF` |
| nLength | DWORD | length from `sCmd` through `sSum` (= data length + 4) |
| sCmd | WORD | command id |
| strData | BYTE[] | command payload |
| sSum | WORD | byte-wise sum of the payload |

### Commands

| Id | Function | Payload |
|---:|----------|---------|
| 1 | List drive letters | none |
| 2 | List directory contents | directory path |
| 3 | Open / run a remote file | file path |
| 4 | Download a file | file path; returned in chunks |
| 5 | Mouse event | `MOUSEEV` struct (action, button, position) |
| 6 | Send screenshot | none; returns a JPEG image |
| 7 | Lock the machine | none |
| 8 | Unlock the machine | none |
| 9 | Delete a remote file | file path |
| 1981 | Connection test | none |

## Design notes

- Server flow: `WSAStartup` → `socket` → `bind` → `listen` → `accept` → `recv` / `send`, wrapped by `CServerSocket`; `CCommand` looks the command id up in a table and dispatches to the handler.
- The client follows MVC: dialogs only do UI, `CClientController` sends commands and routes results back to the owning dialog as window messages; I/O runs on a separate thread, queued through `CEdoyunQueue`.
- Remote desktop viewing polls screenshots (command 6) while mouse actions are sent immediately (command 5).

## License

No license has been declared for this project yet.
