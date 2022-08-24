#include <cstdio>
#include <cstdlib>
#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include "resources.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

class TurboController
{
public:
    TurboController() : state_{false} {} // TODO: retrieve initial state

    auto set_state(bool state) noexcept -> bool
    {
        if (!apply_power_scheme(state))
            return false;
        state_ = state;
        return true;
    }

    auto get_state() const noexcept -> bool
    {
        return state_;
    }

    auto toggle_state() noexcept -> bool
    {
        return set_state(!get_state());
    }

private:
    auto find_trim_guid(char *first, const size_t length) const noexcept -> const char *
    {
        const char *const end = first + length;

        while ((*first != ':') && (first < end))
            ++first;
        ++first;
        ++first;
        if (first >= end)
            return nullptr;

        char *tmp = first;
        while ((*tmp != ' ') && (tmp < end))
            ++tmp;
        if (first >= end)
            return nullptr;

        *tmp = 0;

        return first;
    }

    auto apply_power_scheme(const bool state) const noexcept -> bool
    {
        const size_t PIPE_BUFFER_LENGTH = 128, CMD_BUFFER_LENGTH = 512;
        const char *CMD_AC_ON_F = "powercfg.exe /SETACVALUEINDEX %s 54533251-82be-4824-96c1-47b60b740d00 be337238-0d82-4146-a960-4f3749d470c7 003";
        const char *CMD_DC_ON_F = "powercfg.exe /SETDCVALUEINDEX %s 54533251-82be-4824-96c1-47b60b740d00 be337238-0d82-4146-a960-4f3749d470c7 003";
        const char *CMD_AC_OFF_F = "powercfg.exe /SETACVALUEINDEX %s 54533251-82be-4824-96c1-47b60b740d00 be337238-0d82-4146-a960-4f3749d470c7 000";
        const char *CMD_DC_OFF_F = "powercfg.exe /SETDCVALUEINDEX %s 54533251-82be-4824-96c1-47b60b740d00 be337238-0d82-4146-a960-4f3749d470c7 000";
        const char *CMD_CURRENT_SCHEME = "powercfg.exe -S SCHEME_CURRENT";

        char proc_out[PIPE_BUFFER_LENGTH];
        FILE *fd = ::popen("powercfg.exe /GETACTIVESCHEME", "r");
        auto r = ::fgets(proc_out, PIPE_BUFFER_LENGTH, fd);
        ::pclose(fd);

        if (r != proc_out)
            return false;

        auto guid_str = find_trim_guid(proc_out, PIPE_BUFFER_LENGTH);

        if (!guid_str)
            return false;

        char system_cmd[CMD_BUFFER_LENGTH];
        if (state)
        {
            auto r = ::snprintf(system_cmd, CMD_BUFFER_LENGTH, CMD_AC_ON_F, guid_str);
            if ((r < 0) || (r == CMD_BUFFER_LENGTH))
                return false;

            ::system(system_cmd);

            r = ::snprintf(system_cmd, CMD_BUFFER_LENGTH, CMD_DC_ON_F, guid_str);
            if ((r < 0) || (r == CMD_BUFFER_LENGTH))
                return false;

            ::system(system_cmd);
        }
        else
        {
            auto r = ::snprintf(system_cmd, CMD_BUFFER_LENGTH, CMD_AC_OFF_F, guid_str);
            if ((r < 0) || (r == CMD_BUFFER_LENGTH))
                return false;

            ::system(system_cmd);

            r = ::snprintf(system_cmd, CMD_BUFFER_LENGTH, CMD_DC_OFF_F, guid_str);
            if ((r < 0) || (r == CMD_BUFFER_LENGTH))
                return false;

            ::system(system_cmd);
        }

        ::system(CMD_CURRENT_SCHEME);
        return true;
    }

    bool state_;
};

class TurboNotifyIcon
{
public:
    constexpr static const size_t ID_TRAY_APP_ICON = 5000;
    constexpr static const size_t WM_TRAYICON = (WM_USER + 1);

    TurboNotifyIcon(HINSTANCE hinstance, HWND hwnd, bool state)
        : hinstance_{hinstance}, hwnd_{hwnd}
    {
        ::memset(&notify_icon_data_, 0, sizeof(NOTIFYICONDATA));
        notify_icon_data_.cbSize = sizeof(NOTIFYICONDATA);
        notify_icon_data_.hWnd = hwnd_;
        notify_icon_data_.uID = ID_TRAY_APP_ICON;
        notify_icon_data_.uFlags = NIF_ICON | NIF_MESSAGE;
        notify_icon_data_.uCallbackMessage = WM_TRAYICON;

        notify_icon_data_.hIcon = load_turbo_icon(state);

        ::Shell_NotifyIcon(NIM_ADD, &notify_icon_data_);
        ::DestroyIcon(notify_icon_data_.hIcon);
        notify_icon_data_.hIcon = nullptr;
    }

    auto update(bool state) noexcept -> void
    {
        notify_icon_data_.hIcon = load_turbo_icon(state);

        ::Shell_NotifyIcon(NIM_MODIFY, &notify_icon_data_);
        ::DestroyIcon(notify_icon_data_.hIcon);
        notify_icon_data_.hIcon = nullptr;
    }

    virtual ~TurboNotifyIcon()
    {
        ::Shell_NotifyIcon(NIM_DELETE, &notify_icon_data_);
    }

private:
    auto load_turbo_icon(bool state) const noexcept -> HICON
    {
        if (state)
            return ::LoadIcon(hinstance_, MAKEINTRESOURCE(ICO_TURBO_ON));
        else
            return ::LoadIcon(hinstance_, MAKEINTRESOURCE(ICO_TURBO_OFF));
    }

    const HINSTANCE hinstance_;
    const HWND hwnd_;
    NOTIFYICONDATA notify_icon_data_;
};

class App
{
public:
    App(HINSTANCE hInstance)
        : hinstance_{hInstance}, turbo_ctl_{std::make_unique<TurboController>()}
    {
        turbo_ctl_->set_state(true);
        init_window();
    }

    LRESULT handle_wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case TurboNotifyIcon::WM_TRAYICON:
            if (wParam == TurboNotifyIcon::ID_TRAY_APP_ICON)
            {
                if (lParam == WM_LBUTTONUP)
                {
                    turbo_ctl_->toggle_state();
                    notify_icon_->update(turbo_ctl_->get_state());
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
        default:
            return ::DefWindowProc(hwnd, message, wParam, lParam);
        }
        return 0;
    }

private:
    auto init_window() noexcept -> void
    {
        TCHAR className[] = TEXT("ToggleTurboClass");

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

        notify_icon_ = std::make_unique<TurboNotifyIcon>(hinstance_, hwnd_, turbo_ctl_->get_state());
    }

    auto display_command_help() const noexcept -> void
    {
        int r = ::MessageBox(hwnd_, TEXT("Command to enable the TurboBoost control:\n"
                                         "powercfg.exe -attributes SUB_PROCESSOR be337238-0d82-4146-a960-4f3749d470c7 -ATTRIB_HIDE\n"
                                         "Copy to clipboard?"),
                             TEXT("ToggleTurbo Control"),
                             MB_YESNO | MB_ICONINFORMATION);

        if (r == IDYES)
        {
            const char *cmd = "powercfg.exe -attributes SUB_PROCESSOR be337238-0d82-4146-a960-4f3749d470c7 -ATTRIB_HIDE";
            const size_t len = strlen(cmd) + 1;
            HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, len);
            ::memcpy(::GlobalLock(hMem), cmd, len);
            ::GlobalUnlock(hMem);
            ::OpenClipboard(0);
            ::EmptyClipboard();
            ::SetClipboardData(CF_TEXT, hMem);
            ::CloseClipboard();
        }
    }

    HINSTANCE hinstance_;
    HWND hwnd_;
    std::unique_ptr<TurboController> turbo_ctl_;
    std::unique_ptr<TurboNotifyIcon> notify_icon_;
};

std::unique_ptr<App> app;

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

    app = std::make_unique<App>(hInstance);

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
