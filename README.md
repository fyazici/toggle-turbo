# toggle-turbo

This is a very simple utility tool to enable and disable TurboBoost function of Intel processors **only on Windows**. 

For Linux, alternatives exist: [georgewhewell/undervolt](https://github.com/georgewhewell/undervolt)

The program creates a taskbar icon that toggles the turbo state on left-click. **Right-click closes the program. Note that Windows will keep the last setting even if the program is closed.**

Use the following command to enable turbo-boost management through `powercfg` on Windows:
```
powercfg.exe -attributes SUB_PROCESSOR be337238-0d82-4146-a960-4f3749d470c7 -ATTRIB_HIDE
```

Middle-click on the taskbar icon also shows this command and allows you to copy it to clipboard.

Motivation
==

Laptop CPUs run hot (95-100 C) when loaded. This causes discomfort due to fan noise and micro-stutters in performance. Especially for high-end CPUs, raw performance without turboboost is more than enough for most daily tasks, but the extra edge is needed while doing research or other resource intensive job. I felt the need to switch between on and off states often and quickly, so I developed this tool for personal usage.

Building
==

Since this very simple program is intended to be used only on Windows, just a `build.bat` file is provided. It uses **MinGW GCC**, so if you have it on a different path, modify the first command on the script to add it to path. 

Usage
==

There is (currently) no user interface for this program. Taskbar icon shows the state of turbo (whether on or off). Left-click toggles the state, middle-click shows some information, and right-click closes the program. Put a shortcut to the executable under the startup directory (`Win+R` and type `shell:startup`) to make it run on login automatically.

Todo
==

* Remember the state of turboboost and restore the setting when closed
* Show the existing state of turboboost upon startup (currently shows *disabled* regardless of the actual setting)
