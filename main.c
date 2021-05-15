#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef UNICODE
#define stringcopy wcscpy
#else
#define stringcopy strcpy
#endif

#define ID_TRAY_APP_ICON 5000
#define WM_TRAYICON (WM_USER + 1)

#define TURBO_OFF (0)
#define TURBO_ON (1)

UINT turbo_state = TURBO_OFF;
UINT WM_TASKBARCREATED = 0;

HICON turbo_off_icon;
HICON turbo_on_icon;

HINSTANCE g_hinstance;
HWND g_hwnd;
NOTIFYICONDATA g_notifyIconData;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void ModifyTurboState()
{
    char buf1[512], buf2[512];
    FILE *fd = popen("powercfg.exe /GETACTIVESCHEME", "r");
    fread(buf1, 1, 512, fd);
    pclose(fd);
    char *p1 = buf1, *p2;
    while (*p1 != ':' && *p1 != 0)
        ++p1;
    ++p1;
    ++p1;
    p2 = p1;
    while (*p2 != ' ' && *p2 != 0)
        ++p2;
    *p2 = 0;

    if (turbo_state == TURBO_OFF)
    {
        sprintf(buf2, "powercfg.exe /SETACVALUEINDEX %s 54533251-82be-4824-96c1-47b60b740d00 be337238-0d82-4146-a960-4f3749d470c7 000", p1);
        system(buf2);
        sprintf(buf2, "powercfg.exe /SETDCVALUEINDEX %s 54533251-82be-4824-96c1-47b60b740d00 be337238-0d82-4146-a960-4f3749d470c7 000", p1);
        system(buf2);
        system("powercfg.exe -S SCHEME_CURRENT");
    }
    else
    {
        sprintf(buf2, "powercfg.exe /SETACVALUEINDEX %s 54533251-82be-4824-96c1-47b60b740d00 be337238-0d82-4146-a960-4f3749d470c7 003", p1);
        system(buf2);
        sprintf(buf2, "powercfg.exe /SETDCVALUEINDEX %s 54533251-82be-4824-96c1-47b60b740d00 be337238-0d82-4146-a960-4f3749d470c7 003", p1);
        system(buf2);
        system("powercfg.exe -S SCHEME_CURRENT");
    }
}

void InitWindow()
{
    TCHAR className[] = TEXT("ToggleTurboClass");
    WM_TASKBARCREATED = RegisterWindowMessageA("TaskbarCreated");

    AllocConsole();
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    WNDCLASSEX wnd = {0};

    wnd.hInstance = g_hinstance;
    wnd.lpszClassName = className;
    wnd.lpfnWndProc = WndProc;
    wnd.style = CS_HREDRAW | CS_VREDRAW;
    wnd.cbSize = sizeof(WNDCLASSEX);

    wnd.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wnd.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
    wnd.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE;

    if (!RegisterClassEx(&wnd))
    {
        FatalAppExit(0, TEXT("Couldn't register window class!"));
    }

    g_hwnd = CreateWindowEx(
        0, className,
        TEXT("ToggleTurbo"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        300, 300,
        NULL, NULL,
        g_hinstance, NULL);

    ShowWindow(g_hwnd, SW_HIDE);
}

void InitNotifyIconData()
{
    memset(&g_notifyIconData, 0, sizeof(NOTIFYICONDATA));
    g_notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
    g_notifyIconData.hWnd = g_hwnd;
    g_notifyIconData.uID = ID_TRAY_APP_ICON;
    g_notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE;
    g_notifyIconData.uCallbackMessage = WM_TRAYICON;
    if (turbo_state == TURBO_OFF)
    {
        g_notifyIconData.hIcon = (HICON)LoadImage(g_hinstance, TEXT("ICO_TURBO_OFF"), IMAGE_ICON, 0, 0, 0);
        ;
    }
    else
    {
        g_notifyIconData.hIcon = (HICON)LoadImage(g_hinstance, TEXT("ICO_TURBO_ON"), IMAGE_ICON, 0, 0, 0);
        ;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR args, int iCmdShow)
{
    g_hinstance = hInstance;
    InitWindow();
    InitNotifyIconData();
    Shell_NotifyIcon(NIM_ADD, &g_notifyIconData);
    ModifyTurboState();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Shell_NotifyIcon(NIM_DELETE, &g_notifyIconData);

    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_TASKBARCREATED)
    {
        //printf("taskbar create\n");
        Shell_NotifyIcon(NIM_ADD, &g_notifyIconData);
        return 0;
    }

    switch (message)
    {
    case WM_TRAYICON:
        if (wParam == ID_TRAY_APP_ICON)
        {
            if (lParam == WM_LBUTTONUP)
            {
                turbo_state = 1 - turbo_state;

                ModifyTurboState();
                InitNotifyIconData();
                Shell_NotifyIcon(NIM_MODIFY, &g_notifyIconData);

                //printf("leftup -> new turbo state: %s\n", turbo_state ? "on" : "off");
            }
            else if (lParam == WM_RBUTTONUP)
            {
                //printf("rightup\n");
                PostQuitMessage(0);
            }
            else if (lParam == WM_MBUTTONUP)
            {
                int r = MessageBox(hwnd, TEXT(
                    "Command to enable the TurboBoost control:\n"
                    "powercfg.exe -attributes SUB_PROCESSOR be337238-0d82-4146-a960-4f3749d470c7 -ATTRIB_HIDE\n"
                    "Copy to clipboard?"),
                    TEXT("ToggleTurbo Control"),
                    MB_YESNO | MB_ICONINFORMATION
                );

                if (r == IDYES)
                {
                    const char *cmd = "powercfg.exe -attributes SUB_PROCESSOR be337238-0d82-4146-a960-4f3749d470c7 -ATTRIB_HIDE";
                    const size_t len = strlen(cmd) + 1;
                    HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, len);
                    memcpy(GlobalLock(hMem), cmd, len);
                    GlobalUnlock(hMem);
                    OpenClipboard(0);
                    EmptyClipboard();
                    SetClipboardData(CF_TEXT, hMem);
                    CloseClipboard();
                }
            }
        }
        break;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}
