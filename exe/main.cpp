#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include <iostream>
#include <vector>
#include <filesystem>
#include <optional>

using namespace std;

PCWSTR DLL_NAME = L"WSLHostPatch";

bool listWslHosts(vector<DWORD>* pids)
{
	HANDLE hProcessSnap = nullptr;
	HANDLE hProcess = nullptr;
	PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == nullptr)
	{
		return false;
	}

	if (!Process32First(hProcessSnap, &pe32))
	{
		CloseHandle(hProcessSnap);
		return false;
	}

	do {
		if ((_wcsicmp(pe32.szExeFile, L"wslhost.exe") == 0) || (_wcsicmp(pe32.szExeFile, L"wslrelay.exe") == 0)) {
			pids->push_back(pe32.th32ProcessID);
		}
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);

	return true;
}

wstring* getDllPath()
{
	HMODULE hExe = GetModuleHandle(nullptr);
	if (hExe == nullptr) {
		return nullptr;
	}

	DWORD bufSize = MAX_PATH;
	WCHAR exePath[MAX_PATH];

	if (GetModuleFileName(hExe, exePath, bufSize) >= bufSize) {
		return nullptr;
	}

	auto p = filesystem::path(exePath);
	p.replace_filename(wstring(DLL_NAME) + L".dll");

	return new wstring(p.wstring());
}

HANDLE attachProcess(DWORD pid)
{
	return OpenProcess(
		PROCESS_QUERY_INFORMATION |
		PROCESS_CREATE_THREAD |   // For CreateRemoteThread
		PROCESS_VM_OPERATION |   // For VirtualAllocEx/VirtualFreeEx
		PROCESS_VM_READ |	// For EnumProcessModules
		PROCESS_VM_WRITE |	// For WriteProcessMemory
		SYNCHRONIZE,
		FALSE, pid);
}

bool injectProcess(HANDLE hProcess, const wstring* dllPath)
{
	bool bOk = false; // Assume that the function fails
	HANDLE hThread = nullptr;
	PCWSTR pFilePathRemote = nullptr;

	do
	{
		size_t pathLengthInBytes = (dllPath->size() + 1) * sizeof(wchar_t);
		pFilePathRemote = (PCWSTR)
			VirtualAllocEx(hProcess, nullptr, pathLengthInBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (pFilePathRemote == nullptr) break;

		if (!WriteProcessMemory(hProcess, (LPVOID)pFilePathRemote,
			(PVOID)dllPath->c_str(), pathLengthInBytes, nullptr))
		{
			break;
		}
		HMODULE hKernel = GetModuleHandle(L"kernel32");
		if (hKernel == nullptr) {
			break;
		}

		PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)
			GetProcAddress(hKernel, "LoadLibraryW");
		if (pfnThreadRtn == nullptr) break;

		HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
			pfnThreadRtn, (LPVOID)pFilePathRemote, 0, nullptr);
		if (hThread == nullptr) break;

		WaitForSingleObject(hThread, INFINITE);
		bOk = true;

	} while (false);


	if (pFilePathRemote != nullptr)
		VirtualFreeEx(hProcess, (LPVOID)pFilePathRemote, 0, MEM_RELEASE);

	if (hThread != nullptr)
		CloseHandle(hThread);

	return bOk;
}

optional<HMODULE> getInjectedModule(HANDLE hProcess, const wstring* dllPath)
{
	auto modules = make_unique<HMODULE[]>(1024);
	DWORD cb = sizeof(HMODULE) * 1024;
	DWORD requiredSize = 0;

	if (!EnumProcessModules(hProcess, modules.get(), cb, &requiredSize))
	{
		auto err = GetLastError();
		return nullopt;
	}

	if (requiredSize > cb)
	{
		return nullopt;
	}

	WCHAR path[MAX_PATH];
	for (auto i = 0; i < requiredSize / sizeof(HMODULE); i++)
	{
		if (!GetModuleFileNameEx(hProcess, modules[i], path, sizeof(path) / sizeof(WCHAR)))
		{
			return nullopt;
		}

		if (_wcsicmp(dllPath->c_str(), path) == 0)
		{
			return modules[i];
		}
	}

	return nullptr;
}

int wmain()
{
	auto dllPath = getDllPath();

	wcout << "Dll path: " << *dllPath << endl;

	vector<DWORD> pids;
	listWslHosts(&pids);

	wcout << "Found " << pids.size() << " WSL host" << endl;

	int patchedCount = 0;
	for (const auto& pid : pids)
	{
		HANDLE process = attachProcess(pid);
		auto injected = getInjectedModule(process, dllPath);
		if (!injected.has_value())
		{
			return -1;
		}

		if (injected.value() == nullptr) {
			injectProcess(process, dllPath);
		}

		CloseHandle(process);
		patchedCount++;
	}

	wcout << "Patched " << patchedCount << endl;

	return 0;
}
