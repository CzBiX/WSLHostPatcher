#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <detours/detours.h>

#include <winsock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

static int (PASCAL * OldBind)(SOCKET s, const sockaddr* adr, int namelen) = bind;

int PASCAL NewBind(SOCKET s, const sockaddr* addr, int namelen)
{
    if (addr->sa_family == AF_INET)
    {
        auto addr_in = (sockaddr_in*)addr;
        addr_in->sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else if (addr->sa_family == AF_INET6)
    {
        DWORD ipv6Only = 0;
        setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (PCSTR)&ipv6Only, sizeof(ipv6Only));

        auto addr_in = (sockaddr_in6*)addr;
        addr_in->sin6_addr = IN6ADDR_ANY_INIT;
    }

    return OldBind(s, addr, namelen);
}

BOOL APIENTRY DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DetourRestoreAfterWith();

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)OldBind, NewBind);
        DetourTransactionCommit();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)OldBind, NewBind);
        DetourTransactionCommit();
        break;
    }
    return TRUE;
}
