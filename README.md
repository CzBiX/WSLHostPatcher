# WSLHostPatcher
Dynamic patch `wslhost.exe` to listen port on any interfaces.

# How it work
The localhost port actually forward by wslhost.exe on Windows, but it listen on localhost only.

WSLHostPatcher will scan all wslhost.exe processes, then inject into it to hook `bind` API  listen on any IP.
There is no any background processes neither cost any performance.

# How to use
Download release and unzip it on Windows. Run `WSLHostPatcher.exe` after WSL2 started.

You can also put it in your `.profile`, so it will run automatically.

# How to restore
`wsl --shutdown` or Reboot system.

# Security Consideration
Listen port on any interfaces may cause some security problems. You are on your own.
