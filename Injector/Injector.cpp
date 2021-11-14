// Injector.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
// Разработать DLL Injector использующий CreateRemoteThread. протестировать на "Hello World" динамической библиотеке

#include <iostream>
#include <Windows.h>
#include <tlhelp32.h>

bool InjectDllToTheProcess(DWORD processId, PCWSTR dllPath)
{
	bool res = false;
	HANDLE targetProcess = NULL;
	HANDLE remoteThread = NULL;
	LPWSTR remoteBuff = NULL;

	__try
	{
		targetProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, processId);
		if (targetProcess == NULL)
		{
			__leave;
		}

		int buffSize = (lstrlenW(dllPath) + 1) * sizeof(WCHAR);
		remoteBuff = reinterpret_cast<LPWSTR>(VirtualAllocEx(targetProcess, NULL, buffSize, MEM_COMMIT, PAGE_READWRITE));
		if (remoteBuff == NULL)
		{
			__leave;
		}

		if (!WriteProcessMemory(targetProcess, remoteBuff, dllPath, buffSize, NULL))
		{
			__leave;
		}

		PTHREAD_START_ROUTINE loadLibraryFunction = reinterpret_cast<PTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleW(L"Kernal32"), "LoadLibraryW"));
		if (loadLibraryFunction == NULL)
		{
			__leave;
		}

		remoteThread = CreateRemoteThread(targetProcess, NULL, 0, loadLibraryFunction, remoteBuff, 0, NULL);
		if (remoteThread == NULL)
		{
			__leave;
		}

		WaitForSingleObject(remoteThread, INFINITE);

		res = true;
	}
	__finally
	{
		if (remoteBuff != NULL)
		{
			VirtualFreeEx(targetProcess, remoteBuff, 0, MEM_RELEASE);
		}

		if (remoteThread != NULL)
		{
			CloseHandle(remoteThread);
		}

		if (targetProcess != NULL)
		{
			CloseHandle(targetProcess);
		}
	}

	return res;
}

bool EjectDllFromTheProcess(DWORD processId, PCWSTR dllPath) 
{
	bool res = false;
	HANDLE threadsSnapshot = NULL;
	HANDLE targetProcess = NULL;
	HANDLE remoteThread = NULL;

	__try 
	{
		threadsSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
		if (threadsSnapshot == NULL)
		{
			__leave;
		}

		MODULEENTRY32W me;
		me.dwSize = sizeof(me);
		BOOL fFound = FALSE;
		BOOL fMoreMods = Module32FirstW(threadsSnapshot, &me);
		for (; fMoreMods; fMoreMods = Module32NextW(threadsSnapshot, &me))
		{
			fFound = (lstrcmpiW(me.szModule, dllPath) == 0) || (lstrcmpiW(me.szExePath, dllPath) == 0);
			if (fFound)
			{
				break;
			}
		}
		if (!fFound) 
		{
			__leave;
		}

		targetProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION, FALSE, processId);
		if (targetProcess == NULL)
		{
			__leave;
		}

		PTHREAD_START_ROUTINE pfnThreadRtn = reinterpret_cast<PTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandle(L"Kernel32"), "FreeLibrary"));
		if (pfnThreadRtn == NULL)
		{
			__leave;
		}

		remoteThread = CreateRemoteThread(targetProcess, NULL, 0, pfnThreadRtn, me.modBaseAddr, 0, NULL);
		if (remoteThread == NULL)
		{
			__leave;
		}

		WaitForSingleObject(remoteThread, INFINITE);

		res = true;
	}
	__finally 
	{ 
		if (threadsSnapshot != NULL)
		{
			CloseHandle(threadsSnapshot);
		}

		if (remoteThread != NULL)
		{
			CloseHandle(remoteThread);
		}

		if (targetProcess != NULL)
		{
			CloseHandle(targetProcess);
		}
	}

	return res;
}

int main()
{
	return 0;
}
