# WSLHostPatcher
Dynamic patch WSL2 to listen port on any interfaces.

# Updates
Check out experimental bridge solution at https://github.com/microsoft/WSL/issues/4150#issuecomment-1018524753.

Also [MS official config](https://docs.microsoft.com/en-us/windows/wsl/wsl-config#configuration-setting-for-wslconfig) for WSL 2 in Windows Build 19041 and later.

# How it work
The localhost port actually forward by `wslhost.exe` on Windows, but it listen on localhost only.

WSLHostPatcher will scan all `wslhost.exe` processes, then inject into it to hook `bind` API  listen on any IP.
There is no any background processes neither cost any performance.

# How to use
Download [release](https://github.com/CzBiX/WSLHostPatcher/releases/latest) and unzip it on Windows. Run `WSLHostPatcher.exe` after WSL2 started.
This patch needs to running on every time WSL starts, and only the ports listening after running patch will work.
You can also put it in your `.profile`, so it will run automatically, see [#1](https://github.com/CzBiX/WSLHostPatcher/issues/1).

# How to restore
`wsl --shutdown` or Reboot system.

# Security Consideration
Listen port on any interfaces may cause some security problems. You are on your own.

Anti-virus software may alert, see [discussion](https://github.com/CzBiX/WSLHostPatcher/discussions/16).
