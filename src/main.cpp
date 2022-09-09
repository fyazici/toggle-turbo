#include <memory>
#include "turbo_controller.hpp"
#include "resources.h"

extern "C" {
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
}

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

class App
{
public:
    constexpr static const size_t ID_TRAY_APP_ICON = 5000;
    constexpr static const size_t WM_TRAYICON = (WM_USER + 1);

    App(HINSTANCE hInstance): hinstance_{hInstance}
    {
        init_window();
        init_notify_icon();
    }

    LRESULT handle_wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_TRAYICON:
            if (wParam == ID_TRAY_APP_ICON)
            {
                if (lParam == WM_LBUTTONUP)
                {
                    turbo_ctl_.toggle_state();
                    update_turbo_icon(turbo_ctl_.get_state());
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
            return DefWindowProc(hwnd, message, wParam, lParam);
        }
        return 0;
    }

    virtual ~App() noexcept
    {
        Shell_NotifyIcon(NIM_DELETE, &notify_icon_data_);
    }

private:
    auto init_window() noexcept -> void
    {
        TCHAR className[] = TEXT("ToggleTurboClass");

        WNDCLASSEX wnd;
        memset(&wnd, 0, sizeof(wnd));

        wnd.hInstance = hinstance_;
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

        hwnd_ = CreateWindowEx(
            0, className,
            TEXT("ToggleTurbo"),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            300, 300,
            NULL, NULL,
            hinstance_, NULL);
    }

    auto init_notify_icon() noexcept -> void
    {
        memset(&notify_icon_data_, 0, sizeof(NOTIFYICONDATA));
        notify_icon_data_.cbSize = sizeof(NOTIFYICONDATA);
        notify_icon_data_.hWnd = hwnd_;
        notify_icon_data_.uID = ID_TRAY_APP_ICON;
        notify_icon_data_.uFlags = NIF_ICON | NIF_MESSAGE;
        notify_icon_data_.uCallbackMessage = WM_TRAYICON;

        notify_icon_data_.hIcon = load_turbo_icon(turbo_ctl_.get_state());

        Shell_NotifyIcon(NIM_ADD, &notify_icon_data_);
        DestroyIcon(notify_icon_data_.hIcon);
        notify_icon_data_.hIcon = nullptr;
    }

    auto load_turbo_icon(bool state) const noexcept -> HICON
    {
        if (state)
            return LoadIcon(hinstance_, MAKEINTRESOURCE(ICO_TURBO_ON));
        else
            return LoadIcon(hinstance_, MAKEINTRESOURCE(ICO_TURBO_OFF));
    }

    auto update_turbo_icon(bool state) noexcept -> void
    {
        notify_icon_data_.hIcon = load_turbo_icon(state);

        Shell_NotifyIcon(NIM_MODIFY, &notify_icon_data_);
        DestroyIcon(notify_icon_data_.hIcon);
        notify_icon_data_.hIcon = nullptr;
    }

    auto display_command_help() const noexcept -> void
    {
        int r = MessageBox(hwnd_, TEXT("Command to enable the TurboBoost control:\n"
                                         "powercfg.exe -attributes SUB_PROCESSOR be337238-0d82-4146-a960-4f3749d470c7 -ATTRIB_HIDE\n"
                                         "Copy to clipboard?"),
                             TEXT("ToggleTurbo Control"),
                             MB_YESNO | MB_ICONINFORMATION);

        if (r == IDYES)
        {
            const std::string cmd = "powercfg.exe -attributes SUB_PROCESSOR be337238-0d82-4146-a960-4f3749d470c7 -ATTRIB_HIDE";
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, cmd.size() + 1);
            if (!hMem)
                return;
            auto pMem = GlobalLock(hMem);
            if (!pMem) 
                return;
            memcpy(pMem, cmd.c_str(), cmd.size() + 1);
            GlobalUnlock(hMem);
            OpenClipboard(0);
            EmptyClipboard();
            SetClipboardData(CF_TEXT, hMem);
            CloseClipboard();
        }
    }

    HINSTANCE hinstance_;
    HWND hwnd_;
    NOTIFYICONDATA notify_icon_data_;
    turbo_controller::TurboController turbo_ctl_;
};

std::unique_ptr<App> app;

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    [[maybe_unused]] _In_opt_ HINSTANCE hPrevInstance,
    [[maybe_unused]] _In_ LPSTR lpCmdLine,
    [[maybe_unused]] _In_ int nShowCmd)
{
#ifdef NDEBUG
    AllocConsole();
    ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif

    app = std::make_unique<App>(hInstance);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return app->handle_wndproc(hwnd, message, wParam, lParam);
}
