# WSLHostPatcher
Dynamic patch `wslhost.exe` to listen port on any interface.

# How it work
The localhost port actually forward by wslhost.exe on Windows, but it listen on localhost only.

WSLHostPatcher will scan all wslhost.exe processes, then hook `bind` API to listen on any ip.

# How to use
Download release and unzip it on Windows. Run `WSLHostPatcher.exe` after WSL2 started.

You can also put it in your `.profile`, so it will run automatically.
