#include "DllInjector.h"

//bool InjectDllToTheProcess(DWORD processId, PCWSTR dllPath)
//{
//	bool res = false;
//	HANDLE targetProcess = NULL;
//	HANDLE remoteThread = NULL;
//	LPWSTR remoteBuff = NULL;
//
//	__try
//	{
//		targetProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, processId);
//		if (targetProcess == NULL)
//		{
//			__leave;
//		}
//		/*
//		int buffSize = (lstrlenW(dllPath) + 1) * sizeof(WCHAR);
//		remoteBuff = reinterpret_cast<LPWSTR>(VirtualAllocEx(targetProcess, NULL, buffSize, MEM_COMMIT, PAGE_READWRITE));
//		if (remoteBuff == NULL)
//		{
//			__leave;
//		}
//
//		if (!WriteProcessMemory(targetProcess, remoteBuff, dllPath, buffSize, NULL))
//		{
//			__leave;
//		}
//
//		PTHREAD_START_ROUTINE loadLibraryFunction = reinterpret_cast<PTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleW(L"Kernal32"), "LoadLibraryW"));
//		if (loadLibraryFunction == NULL)
//		{
//			__leave;
//		}
//
//		remoteThread = CreateRemoteThread(targetProcess, NULL, 0, loadLibraryFunction, remoteBuff, 0, NULL);
//		if (remoteThread == NULL)
//		{
//			__leave;
//		}
//
//		WaitForSingleObject(remoteThread, INFINITE);
//
//		res = true;
//	}
//	__finally
//	{
//		if (remoteBuff != NULL)
//		{
//			VirtualFreeEx(targetProcess, remoteBuff, 0, MEM_RELEASE);
//		}
//
//		if (remoteThread != NULL)
//		{
//			CloseHandle(remoteThread);
//		}
//
//		if (targetProcess != NULL)
//		{
//			CloseHandle(targetProcess);
//		}
//	}
//
//	return res;*/
//}

//bool EjectDllFromTheProcess(DWORD processId, PCWSTR dllPath)
//{
//	bool res = false;
//	HANDLE threadsSnapshot = NULL;
//	HANDLE targetProcess = NULL;
//	HANDLE remoteThread = NULL;
//
//	__try
//	{
//		processSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
//		if (threadsSnapshot == NULL)
//		{
//			__leave;
//		}
//
//		MODULEENTRY32W me;
//		me.dwSize = sizeof(me);
//		BOOL fFound = FALSE;
//		BOOL fMoreMods = Module32FirstW(threadsSnapshot, &me);
//		for (; fMoreMods; fMoreMods = Module32NextW(threadsSnapshot, &me))
//		{
//			fFound = (lstrcmpiW(me.szModule, dllPath) == 0) || (lstrcmpiW(me.szExePath, dllPath) == 0);
//			if (fFound)
//			{
//				break;
//			}
//		}
//		if (!fFound)
//		{
//			__leave;
//		}
//
//		targetProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION, FALSE, processId);
//		if (targetProcess == NULL)
//		{
//			__leave;
//		}
//
//		PTHREAD_START_ROUTINE pfnThreadRtn = reinterpret_cast<PTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandle(L"Kernel32"), "FreeLibrary"));
//		if (pfnThreadRtn == NULL)
//		{
//			__leave;
//		}
//
//		remoteThread = CreateRemoteThread(targetProcess, NULL, 0, pfnThreadRtn, me.modBaseAddr, 0, NULL);
//		if (remoteThread == NULL)
//		{
//			__leave;
//		}
//
//		WaitForSingleObject(remoteThread, INFINITE);
//
//		res = true;
//	}
//	__finally
//	{
//		if (threadsSnapshot != NULL)
//		{
//			CloseHandle(threadsSnapshot);
//		}
//
//		if (remoteThread != NULL)
//		{
//			CloseHandle(remoteThread);
//		}
//
//		if (targetProcess != NULL)
//		{
//			CloseHandle(targetProcess);
//		}
//	}
//
//	return res;
//}

DllInjector::DllInjector(DWORD processId, const std::wstring& dllPath)
	: mProcessId(processId)
	, mDllPath(dllPath)
{
}

DllInjector::~DllInjector()
{
	Eject();
}

bool DllInjector::Inject()
{
	if (!OpenProcessForInjection())
	{
		Clear();
		return false;
	}

	if(!CreateRemoteBuffer())
	{
		Clear();
		return false;
	}

	if (!WriteDllPathToRemoteBuffer())
	{
		Clear();
		return false;
	}

	PTHREAD_START_ROUTINE loadLibraryAdress = GetLoadLibraryWAddress();
	if (loadLibraryAdress == NULL)
	{
		Clear();
		return false;
	}

	if (!CreateRemoteThreadInAnotherProcess(loadLibraryAdress, reinterpret_cast<BYTE*>(mRemoteBuff)))
	{
		Clear();
		return false;
	}

	WaitForSingleObject(mRemoteThread, INFINITE);

	Clear();

	return true;
}

bool DllInjector::Eject()
{
	if (!CreateProccessSnapshot())
	{
		Clear();
		return false;
	}

	MODULEENTRY32W me;
	me.dwSize = sizeof(me);
	if (!IsDllLoadedToTheProcess(me))
	{
		Clear();
		return false;
	}

	if (!OpenProcessFotEjection())
	{
		Clear();
		return false;
	}

	PTHREAD_START_ROUTINE freeLibraryAdress = GetFreeLibraryAddress();
	if (freeLibraryAdress == NULL)
	{
		Clear();
		return false;
	}

	if (!CreateRemoteThreadInAnotherProcess(freeLibraryAdress, me.modBaseAddr))
	{
		Clear();
		return false;
	}

	WaitForSingleObject(mRemoteThread, INFINITE);

	return true;
}

void DllInjector::Clear()
{
	if (mProcessSnapshot != NULL)
	{
		CloseHandle(mProcessSnapshot);
	}

	if (mRemoteBuff != NULL)
	{
		VirtualFreeEx(mTargetProcess, mRemoteBuff, 0, MEM_RELEASE);
	}

	if (mRemoteThread != NULL)
	{
		CloseHandle(mRemoteThread);
	}

	if (mTargetProcess != NULL)
	{
		CloseHandle(mTargetProcess);
	}
}

bool DllInjector::OpenProcessForInjection()
{
	mTargetProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, mProcessId);
	return mTargetProcess ? true : false;
}

bool DllInjector::OpenProcessFotEjection()
{
	mTargetProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION, FALSE, mProcessId);
	return mTargetProcess ? true : false;
}

bool DllInjector::CreateRemoteThreadInAnotherProcess(PTHREAD_START_ROUTINE function, BYTE* adress)
{
	mRemoteThread = CreateRemoteThread(mTargetProcess, NULL, 0, function, mRemoteBuff, 0, NULL);
	return mRemoteThread ? true : false;
}

bool DllInjector::CreateRemoteBuffer()
{
	mRemoteBuffSize = (mDllPath.size() + 1) * sizeof(WCHAR);
	mRemoteBuff = reinterpret_cast<LPWSTR>(VirtualAllocEx(mTargetProcess, NULL, mRemoteBuffSize, MEM_COMMIT, PAGE_READWRITE));
	return mRemoteBuff ? true : false;
}

bool DllInjector::WriteDllPathToRemoteBuffer()
{
	return WriteProcessMemory(mTargetProcess, mRemoteBuff, mDllPath.c_str(), mRemoteBuffSize, NULL);
}

PTHREAD_START_ROUTINE DllInjector::GetLoadLibraryWAddress()
{
	return reinterpret_cast<PTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleW(L"Kernel32"), "LoadLibraryW"));
}

PTHREAD_START_ROUTINE DllInjector::GetFreeLibraryAddress()
{
	return reinterpret_cast<PTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleW(L"Kernel32"), "FreeLibrary"));
}

bool DllInjector::CreateProccessSnapshot()
{
	mProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, mProcessId);
	return mProcessSnapshot ? true : false;
}

bool DllInjector::IsDllLoadedToTheProcess(MODULEENTRY32W& entry)
{
	BOOL fMoreMods = Module32FirstW(mProcessSnapshot, &entry);
	for (; fMoreMods; fMoreMods = Module32NextW(mProcessSnapshot, &entry))
	{
		if (!lstrcmpiW(entry.szModule, mDllPath.c_str()) || !lstrcmpiW(entry.szExePath, mDllPath.c_str()))
		{
			return true;
		}
	}
	return false;
}
