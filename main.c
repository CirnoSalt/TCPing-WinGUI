// 启用Unicode支持
#define UNICODE
#define _UNICODE

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <commctrl.h>
// #include <richedit.h> // 不再需要
#include <windowsx.h> 

#pragma comment(lib, "ws2_32.lib")

// 控件ID
#define IDC_IPEDIT         101
#define IDC_PORTEDIT       102
#define IDC_COUNTEDIT      103
#define IDC_LOGEDIT        104
#define IDC_STARTBUTTON    105
#define IDC_STOPBUTTON     106

#define ID_MENU_COPY       107
#define ID_MENU_SELECT_ALL 108
#define IDI_APPICON        101

// 自定义消息
#define WM_PING_COMPLETE (WM_APP + 1)
#define WM_APPEND_LOG    (WM_APP + 2)

// 全局变量
HWND hIpEdit, hPortEdit, hCountEdit, hLogEdit, hStartButton, hStopButton;
HFONT hFont = NULL;
HFONT hFontBold = NULL;
volatile int g_running = 0;

typedef struct _PingStats {
    int pingCount, successCount, failCount;
    double totalTime, minTime, maxTime;
    int maxPingLimit;
} PingStats;

// 函数声明
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void AppendLog(const wchar_t* text);
void StartPing();
void StopPing();
void PingThreadCleanup(PingStats* stats); 
DWORD WINAPI PingThread(LPVOID lpParam);
int TCPPing(const wchar_t* host, int port, double* elapsed);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    INITCOMMONCONTROLSEX icex = { .dwSize = sizeof(icex), .dwICC = ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"TCPingWindowClass";
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    wc.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        0, L"TCPingWindowClass", L"TCP Ping 工具",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 650,
        NULL, NULL, hInstance, NULL
    );

    //不再需要加载RichEdit库
    // LoadLibrary(L"msftedit.dll");

    NONCLIENTMETRICSW ncm = {0};
    ncm.cbSize = sizeof(NONCLIENTMETRICSW);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0);
    
    HDC hdc = GetDC(NULL);
    ncm.lfMessageFont.lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);

    hFont = CreateFontIndirectW(&ncm.lfMessageFont);
    ncm.lfMessageFont.lfWeight = FW_BOLD;
    hFontBold = CreateFontIndirectW(&ncm.lfMessageFont);

    // --- UI布局 ---
    CreateWindowW(L"Button", L"参数设置", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 15, 15, 760, 140, hwnd, NULL, hInstance, NULL);
    CreateWindowW(L"Static", L"IP/域名:", WS_CHILD | WS_VISIBLE | SS_LEFT, 40, 45, 100, 35, hwnd, NULL, hInstance, NULL);
    hIpEdit = CreateWindowW(L"Edit", L"360.com", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 150, 45, 250, 35, hwnd, (HMENU)IDC_IPEDIT, hInstance, NULL);
    CreateWindowW(L"Static", L"端口:", WS_CHILD | WS_VISIBLE | SS_LEFT, 40, 90, 100, 35, hwnd, NULL, hInstance, NULL);
    hPortEdit = CreateWindowW(L"Edit", L"80", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 150, 90, 80, 35, hwnd, (HMENU)IDC_PORTEDIT, hInstance, NULL);
    CreateWindowW(L"Static", L"次数:", WS_CHILD | WS_VISIBLE | SS_LEFT, 260, 90, 80, 35, hwnd, NULL, hInstance, NULL);
    hCountEdit = CreateWindowW(L"Edit", L"4", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 410, 90, 80, 35, hwnd, (HMENU)IDC_COUNTEDIT, hInstance, NULL);
    hStartButton = CreateWindowW(L"Button", L"开始", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 550, 42, 180, 40, hwnd, (HMENU)IDC_STARTBUTTON, hInstance, NULL);
    hStopButton = CreateWindowW(L"Button", L"停止", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED, 550, 88, 180, 40, hwnd, (HMENU)IDC_STOPBUTTON, hInstance, NULL);
    CreateWindowW(L"Button", L"日志输出", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 15, 165, 760, 420, hwnd, NULL, hInstance, NULL);
    
    // 使用标准的EDIT控件
    hLogEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY, 30, 195, 730, 375, hwnd, (HMENU)IDC_LOGEDIT, hInstance, NULL);

    for (HWND hChild = GetTopWindow(hwnd); hChild != NULL; hChild = GetNextWindow(hChild, GW_HWNDNEXT)) {
        SendMessage(hChild, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
    SendMessage(hStartButton, WM_SETFONT, (WPARAM)hFontBold, TRUE);
    SendMessage(hStopButton, WM_SETFONT, (WPARAM)hFontBold, TRUE);

    SendMessageW(hLogEdit, EM_SETLIMITTEXT, 0, 0); // 对标准EDIT控件也有效

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    WSACleanup();
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);

            // "暴力"重绘修复: 如果日志框更新或滚动，则强制重绘整个窗口以清除视觉残留
            if (wmId == IDC_LOGEDIT && (wmEvent == EN_UPDATE || wmEvent == EN_VSCROLL)) {
                InvalidateRect(hwnd, NULL, TRUE);
            }

            // 处理菜单和按钮点击
            switch(wmId) {
                case IDC_STARTBUTTON: StartPing(); break;
                case IDC_STOPBUTTON: StopPing(); break;
                case ID_MENU_COPY: SendMessage(hLogEdit, WM_COPY, 0, 0); break;
                case ID_MENU_SELECT_ALL: SendMessage(hLogEdit, EM_SETSEL, 0, -1); break;
            }
            break;
        }
        case WM_CONTEXTMENU: {
            if ((HWND)wParam == hLogEdit) {
                HMENU hMenu = CreatePopupMenu();
                AppendMenuW(hMenu, MF_STRING, ID_MENU_COPY, L"复制");
                AppendMenuW(hMenu, MF_STRING, ID_MENU_SELECT_ALL, L"全选");
                
                // 使用 EM_GETSEL 兼容标准EDIT控件
                DWORD start, end;
                SendMessage(hLogEdit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
                if (start == end) {
                    EnableMenuItem(hMenu, ID_MENU_COPY, MF_BYCOMMAND | MF_GRAYED);
                }

                TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hwnd, NULL);
                DestroyMenu(hMenu);
            }
            break;
        }
        case WM_PING_COMPLETE:
            PingThreadCleanup((PingStats*)lParam);
            break;
        case WM_APPEND_LOG: {
            wchar_t* logText = (wchar_t*)lParam;
            int len = GetWindowTextLengthW(hLogEdit);
            SendMessageW(hLogEdit, EM_SETSEL, (WPARAM)len, (LPARAM)len);
            SendMessageW(hLogEdit, EM_REPLACESEL, 0, (LPARAM)logText);
            free(logText);
            break;
        }
        case WM_DESTROY: {
            StopPing();
            if (hFont) DeleteObject(hFont);
            if (hFontBold) DeleteObject(hFontBold);
            PostQuitMessage(0);
            break;
        }
        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = 800;
            mmi->ptMinTrackSize.y = 650;
            mmi->ptMaxTrackSize.x = 800;
            mmi->ptMaxTrackSize.y = 650;
            break;
        }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void AppendLog(const wchar_t* text) {
    size_t textLen = wcslen(text);
    size_t bufferLen = textLen + 3; // For \r\n
    wchar_t* buffer = (wchar_t*)malloc(bufferLen * sizeof(wchar_t));
    if (buffer) {
        wcscpy_s(buffer, bufferLen, text);
        wcscat_s(buffer, bufferLen, L"\r\n");
        PostMessageW(GetParent(hLogEdit), WM_APPEND_LOG, 0, (LPARAM)buffer);
    }
}

void StartPing() {
    if (g_running) return;
    
    SetWindowTextW(hLogEdit, L"");
    
    wchar_t ip[256];
    wchar_t portStr[32];
    wchar_t countStr[32];
    GetWindowTextW(hIpEdit, ip, sizeof(ip)/sizeof(ip[0]));
    GetWindowTextW(hPortEdit, portStr, sizeof(portStr)/sizeof(portStr[0]));
    GetWindowTextW(hCountEdit, countStr, sizeof(countStr)/sizeof(countStr[0]));
    
    int port = _wtoi(portStr);
    int maxPingLimit = _wtoi(countStr);
    
    if (wcslen(ip) == 0 || port <= 0 || port > 65535) {
        MessageBoxW(NULL, L"请输入有效的IP和端口（1-65535）", L"错误", MB_ICONERROR);
        return;
    }
    
    wchar_t startMsg[512];
    _snwprintf(startMsg, sizeof(startMsg)/sizeof(startMsg[0]), L"正在 Ping %s (端口 %d)...", ip, port);
    AppendLog(startMsg);
    
    PingStats* initialStats = (PingStats*)malloc(sizeof(PingStats));
    if (!initialStats) return;
    *initialStats = (PingStats){0};
    initialStats->minTime = 999999.0;
    initialStats->maxPingLimit = maxPingLimit;

    struct PingParams { wchar_t host[256]; int port; PingStats* stats; };
    struct PingParams* params = (struct PingParams*)malloc(sizeof(struct PingParams));
    if(!params) { free(initialStats); return; }

    wcscpy_s(params->host, sizeof(params->host)/sizeof(params->host[0]), ip);
    params->port = port;
    params->stats = initialStats;

    g_running = 1;
    EnableWindow(hStartButton, FALSE);
    EnableWindow(hStopButton, TRUE);

    HANDLE hThread = CreateThread(NULL, 0, PingThread, params, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
    } else {
        free(params->stats);
        free(params);
        g_running = 0;
        EnableWindow(hStartButton, TRUE);
        EnableWindow(hStopButton, FALSE);
    }
}

void StopPing() {
    if (!g_running) return;
    EnableWindow(hStopButton, FALSE);
    g_running = 0;
}

void PingThreadCleanup(PingStats* finalStats) {
    g_running = 0;
    EnableWindow(hStartButton, TRUE);
    EnableWindow(hStopButton, FALSE);

    wchar_t lineBuffer[512];

    AppendLog(L"");
    AppendLog(L"Ping 统计信息:");
    
    if (finalStats->pingCount > 0) {
        _snwprintf(lineBuffer, sizeof(lineBuffer)/sizeof(lineBuffer[0]),
            L"    数据包: 已发送 = %d, 已接收 = %d, 丢失 = %d (%.0f%% 丢失)",
            finalStats->pingCount, finalStats->successCount, finalStats->failCount, 
            (finalStats->pingCount > 0) ? (finalStats->failCount * 100.0 / finalStats->pingCount) : 0);
        AppendLog(lineBuffer);

        _snwprintf(lineBuffer, sizeof(lineBuffer)/sizeof(lineBuffer[0]),
            L"往返行程的估计时间(以毫秒为单位):");
        AppendLog(lineBuffer);
        
        _snwprintf(lineBuffer, sizeof(lineBuffer)/sizeof(lineBuffer[0]),
            L"    最短 = %.0fms, 最长 = %.0fms, 平均 = %.0fms",
            finalStats->minTime == 999999.0 ? 0 : finalStats->minTime, finalStats->maxTime, 
            (finalStats->successCount > 0) ? (finalStats->totalTime / finalStats->successCount) : 0);
        AppendLog(lineBuffer);

    } else {
        wcscpy_s(lineBuffer, sizeof(lineBuffer)/sizeof(lineBuffer[0]), L"    Ping 已停止。");
        AppendLog(lineBuffer);
    }
    
    free(finalStats);
}

DWORD WINAPI PingThread(LPVOID lpParam) {
    struct PingParams { wchar_t host[256]; int port; PingStats* stats; };
    struct PingParams* params = (struct PingParams*)lpParam;
    
    wchar_t host[256];
    wcscpy_s(host, sizeof(host)/sizeof(host[0]), params->host);
    int port = params->port;
    PingStats* currentStats = params->stats;
    free(params);

    while (g_running && (currentStats->maxPingLimit == 0 || currentStats->pingCount < currentStats->maxPingLimit)) {
        double elapsed;
        int result = TCPPing(host, port, &elapsed);
        
        if (!g_running) break;

        currentStats->pingCount++;
        wchar_t log[256];
        if (result == 0) {
            currentStats->successCount++;
            currentStats->totalTime += elapsed;
            if (elapsed < currentStats->minTime) currentStats->minTime = elapsed;
            if (elapsed > currentStats->maxTime) currentStats->maxTime = elapsed;
            
            _snwprintf(log, sizeof(log)/sizeof(log[0]), L"来自 %s 的回复: 时间=%.0fms", host, elapsed);
            AppendLog(log);
        } else {
            currentStats->failCount++;
            AppendLog(L"请求超时。");
        }
        
        for(int i = 0; i < 10 && g_running; ++i) {
            Sleep(100);
        }
    }
    
    PostMessage(GetParent(hLogEdit), WM_PING_COMPLETE, 0, (LPARAM)currentStats);

    return 0;
}

int TCPPing(const wchar_t* host, int port, double* elapsed) {
    SOCKET sock = INVALID_SOCKET;
    LARGE_INTEGER frequency, start, end;
    
    QueryPerformanceFrequency(&frequency);

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return -1;
    
    DWORD timeout = 2000;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    
    struct addrinfoW hints, *result = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    wchar_t portStr[16];
    _itow_s(port, portStr, sizeof(portStr)/sizeof(portStr[0]), 10);

    if (GetAddrInfoW(host, portStr, &hints, &result) != 0) {
        closesocket(sock);
        return -1;
    }
    
    struct sockaddr_in serverAddr;
    memcpy(&serverAddr, result->ai_addr, sizeof(struct sockaddr_in));
    FreeAddrInfoW(result);

    QueryPerformanceCounter(&start);
    int connResult = connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    QueryPerformanceCounter(&end);
    
    closesocket(sock);
    
    if (connResult == SOCKET_ERROR) return -1;
    
    *elapsed = (double)(end.QuadPart - start.QuadPart) * 1000.0 / frequency.QuadPart;
    return 0;
}