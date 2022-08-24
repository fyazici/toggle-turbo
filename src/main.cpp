#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <array>
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
    static auto query_power_scheme() noexcept -> const char *
    {
        std::array<char, 128> proc_buf;
        {
            auto proc_fd = std::unique_ptr<FILE, decltype(&pclose)>(popen("powercfg.exe /GETACTIVESCHEME", "r"), pclose);
            auto r = ::fgets(proc_buf.data(), proc_buf.size(), proc_fd.get());
            if (r != proc_buf.data())
                return nullptr;
        }

        auto guid_start = std::find(proc_buf.begin(), proc_buf.end(), ':');
        if (guid_start == proc_buf.end())
            return nullptr;
        guid_start += 2;

        auto guid_end = std::find(guid_start, proc_buf.end(), ' ');
        if (guid_end == proc_buf.end())
            return nullptr;
        *guid_end = 0;
        return guid_start;
    }

    static auto format_and_invoke(const char *guid, const bool state) noexcept -> bool
    {
        const char *POWER_CMD_TURBO = "powercfg.exe /SET%sVALUEINDEX %s 54533251-82be-4824-96c1-47b60b740d00 be337238-0d82-4146-a960-4f3749d470c7 %s";
        const char *POWER_CMD_SET = "powercfg.exe -S SCHEME_CURRENT";

        std::array<char, 512> cmd_buf;
        auto r = ::snprintf(cmd_buf.data(), cmd_buf.size(), POWER_CMD_TURBO, "AC", guid, ((state) ? ("003") : ("000")));
        if ((r < 0) || (r == cmd_buf.size()))
            return false;
        ::system(cmd_buf.data());

        r = ::snprintf(cmd_buf.data(), cmd_buf.size(), POWER_CMD_TURBO, "DC", guid, ((state) ? ("003") : ("000")));
        if ((r < 0) || (r == cmd_buf.size()))
            return false;
        ::system(cmd_buf.data());

        ::system(POWER_CMD_SET);
        return true;
    }

    static auto apply_power_scheme(const bool state) noexcept -> bool
    {
        auto guid_start = query_power_scheme();
        if (!guid_start)
            return false;

        return format_and_invoke(guid_start, state);
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
