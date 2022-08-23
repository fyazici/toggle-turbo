#include <cstdio>

extern "C" {
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

class TurboController
{
public:
    bool set_state(bool state)
    {
        const size_t PIPE_BUFFER_LENGTH = 128, CMD_BUFFER_LENGTH = 512;
        const char *CMD_AC_ON_F = "powercfg.exe /SETACVALUEINDEX %s 54533251-82be-4824-96c1-47b60b740d00 be337238-0d82-4146-a960-4f3749d470c7 003";
        const char *CMD_DC_ON_F = "powercfg.exe /SETDCVALUEINDEX %s 54533251-82be-4824-96c1-47b60b740d00 be337238-0d82-4146-a960-4f3749d470c7 003";
        const char *CMD_AC_OFF_F = "powercfg.exe /SETACVALUEINDEX %s 54533251-82be-4824-96c1-47b60b740d00 be337238-0d82-4146-a960-4f3749d470c7 000";
        const char *CMD_DC_OFF_F = "powercfg.exe /SETDCVALUEINDEX %s 54533251-82be-4824-96c1-47b60b740d00 be337238-0d82-4146-a960-4f3749d470c7 000";
        const char *CMD_CURRENT_SCHEME = "powercfg.exe -S SCHEME_CURRENT";

        char proc_out[PIPE_BUFFER_LENGTH];
        FILE *fd = ::popen("powercfg.exe /GETACTIVESCHEME", "r");
        ::fgets(proc_out, PIPE_BUFFER_LENGTH, fd);
        ::pclose(fd);

        auto guid_str = find_trim_guid(proc_out, PIPE_BUFFER_LENGTH);

        char system_cmd[CMD_BUFFER_LENGTH];
        if (state)
        {
            ::snprintf(system_cmd, CMD_BUFFER_LENGTH, CMD_AC_ON_F, guid_str);
            ::system(system_cmd);
            ::snprintf(system_cmd, CMD_BUFFER_LENGTH, CMD_DC_ON_F, guid_str);
            ::system(system_cmd);
        }
        else
        {
            ::snprintf(system_cmd, CMD_BUFFER_LENGTH, CMD_AC_OFF_F, guid_str);
            ::system(system_cmd);
            ::snprintf(system_cmd, CMD_BUFFER_LENGTH, CMD_DC_OFF_F, guid_str);
            ::system(system_cmd);
        }

        ::system(CMD_CURRENT_SCHEME);
        state_ = state;
        return true;
    }

    auto get_state() -> bool {
        return state_;
    }

    void toggle_state()
    {
        set_state(!state_);
    }

    TurboController() : state_{false} {} // TODO: retrieve initial state

private:
    bool state_;

    const char *find_trim_guid(char * first, size_t length)
    {
        const char * const end = first + length;
        char *tmp;
        while ((*first != ':') && (*first != 0) && (first != end))
            ++first;
        ++first;
        ++first;
        tmp = first;
        while ((*tmp != ' ') && (*tmp != 0) && (tmp != end))
            ++tmp;
        *tmp = 0;
        return first;
    }
};

class App
{
private:
    TurboController ctl_;

public:
    constexpr static const size_t ID_TRAY_APP_ICON = 5000;
    constexpr static const size_t WM_TRAYICON = (WM_USER + 1);

    UINT WM_TASKBARCREATED = 0;

    HINSTANCE hinstance_;
    HWND hwnd_;
    NOTIFYICONDATA notify_icon_data_;

    App(HINSTANCE hInstance) : hinstance_{hInstance} 
    {
        ctl_.set_state(true);
    }

    void init_window()
    {
        TCHAR className[] = TEXT("ToggleTurboClass");
        WM_TASKBARCREATED = RegisterWindowMessageA("TaskbarCreated");

        WNDCLASSEX wnd;
        ::memset(&wnd, 0, sizeof(wnd));

        wnd.hInstance = hinstance_;
        wnd.lpszClassName = className;
        wnd.lpfnWndProc = WndProc;
        wnd.style = CS_HREDRAW | CS_VREDRAW;
        wnd.cbSize = sizeof(WNDCLASSEX);

        wnd.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
        wnd.hIconSm = ::LoadIcon(NULL, IDI_APPLICATION);
        wnd.hCursor = ::LoadCursor(NULL, IDC_ARROW);
        wnd.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE;

        if (!RegisterClassEx(&wnd))
        {
            ::FatalAppExit(0, TEXT("Couldn't register window class!"));
        }

        hwnd_ = ::CreateWindowEx(
            0, className,
            TEXT("ToggleTurbo"),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            300, 300,
            NULL, NULL,
            hinstance_, NULL);
    }

    void init_notifyicon()
    {
        ::memset(&notify_icon_data_, 0, sizeof(NOTIFYICONDATA));
        notify_icon_data_.cbSize = sizeof(NOTIFYICONDATA);
        notify_icon_data_.hWnd = hwnd_;
        notify_icon_data_.uID = ID_TRAY_APP_ICON;
        notify_icon_data_.uFlags = NIF_ICON | NIF_MESSAGE;
        notify_icon_data_.uCallbackMessage = WM_TRAYICON;
        if (ctl_.get_state())
        {
            notify_icon_data_.hIcon = (HICON)::LoadImage(hinstance_, TEXT("ICO_TURBO_ON"), IMAGE_ICON, 0, 0, 0);
        }
        else
        {
            notify_icon_data_.hIcon = (HICON)::LoadImage(hinstance_, TEXT("ICO_TURBO_OFF"), IMAGE_ICON, 0, 0, 0);
        }
        ::Shell_NotifyIcon(NIM_ADD, &notify_icon_data_);
    }

    void update_notifyicon()
    {
        HICON old_icon = notify_icon_data_.hIcon;
        if (ctl_.get_state())
        {
            notify_icon_data_.hIcon = (HICON)::LoadImage(hinstance_, TEXT("ICO_TURBO_ON"), IMAGE_ICON, 0, 0, 0);
        }
        else
        {
            notify_icon_data_.hIcon = (HICON)::LoadImage(hinstance_, TEXT("ICO_TURBO_OFF"), IMAGE_ICON, 0, 0, 0);
        }
        ::Shell_NotifyIcon(NIM_MODIFY, &notify_icon_data_);
        DestroyIcon(old_icon);
    }

    void display_command_help()
    {
        int r = ::MessageBox(hwnd_, TEXT(
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
            HGLOBAL hMem =  ::GlobalAlloc(GMEM_MOVEABLE, len);
            ::memcpy(::GlobalLock(hMem), cmd, len);
            ::GlobalUnlock(hMem);
            ::OpenClipboard(0);
            ::EmptyClipboard();
            ::SetClipboardData(CF_TEXT, hMem);
            ::CloseClipboard();
        }
    }

    LRESULT handle_wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_TASKBARCREATED)
        {
            ::Shell_NotifyIcon(NIM_ADD, &notify_icon_data_);
            return 0;
        }

        switch (message)
        {
        case WM_TRAYICON:
            if (wParam == ID_TRAY_APP_ICON)
            {
                if (lParam == WM_LBUTTONUP)
                {
                    ctl_.toggle_state();
                    update_notifyicon();
                }
                else if (lParam == WM_RBUTTONUP)
                {
                    PostQuitMessage(0);
                }
                else if (lParam == WM_MBUTTONUP)
                {
                    display_command_help();
                }
            }
            break;
        }
        return ::DefWindowProc(hwnd, message, wParam, lParam);
    }

    virtual ~App()
    {
        ::Shell_NotifyIcon(NIM_DELETE, &notify_icon_data_);
    }
};

namespace {
App *app;
}

int WINAPI WinMain(
    HINSTANCE hInstance, 
    [[maybe_unused]] HINSTANCE hPrevInstance, 
    [[maybe_unused]] LPSTR args, 
    [[maybe_unused]] int iCmdShow)
{
    #ifdef NDEBUG
    ::AllocConsole();
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
    #endif

    App app_(hInstance);
    app = &app_;

    app->init_window();
    app->init_notifyicon();

    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    
    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return app->handle_wndproc(hwnd, message, wParam, lParam);
}
