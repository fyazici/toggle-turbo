#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>
#include <memory>

/* Use Windows Power API if available, else fallback to powercfg */
#ifndef TURBO_USE_POWERAPI
#if (_WIN32_WINNT >= 0x0600)
#define TURBO_USE_POWERAPI (1)
#else
#define TURBO_USE_POWERAPI (0)
#endif
#endif

extern "C" {
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef TURBO_USE_POWERAPI
#include <powrprof.h>
#endif
}

namespace turbo_controller
{
namespace detail
{
enum class TurboMode: int
{
    Disabled=0,
    Enabled,
    Aggressive,
    EfficientEnabled,
    EfficientAggressive,
    AggressiveAtGuaranteed,
    EfficientAggressiveAtGuaranteed,
};

static constexpr TurboMode PreferredTurboMode = TurboMode::EfficientAggressive;

template <typename T>
concept turbo_backend = requires(TurboMode m) {
    { T::set_turbo_mode(m) };
    { T::get_turbo_mode() } -> std::same_as<TurboMode>;
};

template <turbo_backend TurboBackend>
class _TurboController
{
public:
    auto set_state(bool state) noexcept -> void
    {
        if (state)
            return TurboBackend::set_turbo_mode(PreferredTurboMode);
        else
            return TurboBackend::set_turbo_mode(TurboMode::Disabled);
    }

    auto get_state() const noexcept -> bool
    {
        return (TurboBackend::get_turbo_mode() != TurboMode::Disabled);
    }

    auto toggle_state() noexcept -> void
    {
        set_state(!get_state());
    }
};

#if TURBO_USE_POWERAPI
class TurboBackendPowerAPI
{
public:
    static auto get_turbo_mode() noexcept -> TurboMode
    {
        const std::string SETTING_KEY = "Current AC Power Setting Index";

        DWORD value_index;

        auto scheme = query_power_scheme();
        if (!scheme)
            return TurboMode::Disabled;

        auto r = PowerReadACValueIndex(
            nullptr, 
            scheme.get(), 
            &GUID_PROCESSOR_SETTINGS_SUBGROUP, 
            &GUID_PROCESSOR_PERF_BOOST_MODE, 
            &value_index    
        );

        if (r)
            return TurboMode::Disabled;

        return (TurboMode)value_index;
    }

    static auto set_turbo_mode(const TurboMode mode) noexcept -> void
    {
        auto scheme = query_power_scheme();
        if (!scheme)
            return;
        
        PowerWriteACValueIndex(
            nullptr,
            scheme.get(),
            &GUID_PROCESSOR_SETTINGS_SUBGROUP,
            &GUID_PROCESSOR_PERF_BOOST_MODE,
            (int)mode
        );

        PowerWriteDCValueIndex(
            nullptr,
            scheme.get(),
            &GUID_PROCESSOR_SETTINGS_SUBGROUP,
            &GUID_PROCESSOR_PERF_BOOST_MODE,
            (int)mode
        );

        PowerSetActiveScheme(nullptr, scheme.get());
    }

private:
    static auto query_power_scheme() noexcept -> std::unique_ptr<GUID, decltype(&LocalFree)>
    {
        GUID *guid = nullptr;
        PowerGetActiveScheme(nullptr, &guid);
        return { guid, &LocalFree };
    }
};

using TurboController = _TurboController<TurboBackendPowerAPI>;

#else /* TURBO_USE_POWERAPI */

class TurboBackendPowercfg
{
public:
    static auto get_turbo_mode() noexcept -> TurboMode
    {
        const std::string POWER_CMD_GET_TURBO = "powercfg.exe /QUERY %s 54533251-82be-4824-96c1-47b60b740d00 be337238-0d82-4146-a960-4f3749d470c7";
        const std::string SETTING_KEY = "Current AC Power Setting Index";

        auto scheme = query_power_scheme();
        auto cmd = format(POWER_CMD_GET_TURBO.c_str(), scheme.c_str());
        auto out = execute_popen(cmd);

        auto idx = out.find(SETTING_KEY);
        if (idx == std::string::npos)
            return TurboMode::Disabled;
        idx += SETTING_KEY.size() + 2;

        int value = std::stoi(out.data() + idx, nullptr, 16);
        return (TurboMode)value;
    }

    static auto set_turbo_mode(const TurboMode mode) noexcept -> void
    {
        auto scheme = query_power_scheme();
        if (!scheme.empty())
            format_and_invoke(scheme, mode);
    }

private:
    template <typename ...Args>
    static auto format(const std::string &fmt, Args ...args) -> std::string
    {
        std::string out;
        auto sz = snprintf(nullptr, 0, fmt.c_str(), std::forward<Args>(args)...);
        out.resize(sz + 1);
        snprintf(out.data(), out.size(), fmt.c_str(), std::forward<Args>(args)...);
        return out;
    }

    static auto execute_popen(const std::string &cmd) -> std::string
    {
        char buf[128];
        std::string proc_out;
        auto proc_fd = std::unique_ptr<FILE, decltype(&_pclose)>(_popen(cmd.c_str(), "r"), _pclose);
        while (!feof(proc_fd.get()))
        {
            if (fgets(buf, 128, proc_fd.get()))
            {
                proc_out += buf;
            }
        }
        return proc_out;
    }

    static auto query_power_scheme() noexcept -> std::string
    {
        const std::string POWER_CMD_GET_CURRENT = "powercfg.exe /GETACTIVESCHEME";

        auto out = execute_popen(POWER_CMD_GET_CURRENT);

        auto guid_start = out.find(':');
        if (guid_start == std::string::npos)
            return {};
        guid_start += 2;

        auto guid_end = out.find(' ', guid_start);
        if (guid_end == std::string::npos)
            return {};
        return out.substr(guid_start, guid_end - guid_start);
    }

    static auto format_and_invoke(const std::string &scheme, const TurboMode mode) noexcept -> bool
    {
        const std::string POWER_CMD_SET_TURBO = "powercfg.exe /SET%sVALUEINDEX %s 54533251-82be-4824-96c1-47b60b740d00 be337238-0d82-4146-a960-4f3749d470c7 %d";
        const std::string POWER_CMD_SET_CURRENT = "powercfg.exe /SETACTIVE SCHEME_CURRENT";

        auto cmd = format(POWER_CMD_SET_TURBO.c_str(), "AC", scheme.c_str(), (int)mode);
        system(cmd.c_str());

        cmd = format(POWER_CMD_SET_TURBO.c_str(), "DC", scheme.c_str(), (int)mode);
        system(cmd.c_str());

        system(POWER_CMD_SET_CURRENT.c_str());
        return true;
    }
};
#warning "this is deprecated"
using TurboController = _TurboController<TurboBackendPowercfg>;

#endif /* TURBO_USE_POWERAPI */
}

using TurboController = detail::TurboController;

}
