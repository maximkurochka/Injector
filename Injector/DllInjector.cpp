#include "DllInjector.h"

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
