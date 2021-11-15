#pragma once

#include <string>
#include <Windows.h>
#include <tlhelp32.h>

class DllInjector
{
public:
	explicit DllInjector(DWORD processId, const std::wstring& dllPath);
	~DllInjector();

	bool Inject();
	bool Eject();

private:
	void Clear();

	bool OpenProcessForInjection();
	bool OpenProcessFotEjection();

	bool CreateRemoteThreadInAnotherProcess(PTHREAD_START_ROUTINE function, BYTE* adress);

	bool CreateRemoteBuffer();
	bool WriteDllPathToRemoteBuffer();

	PTHREAD_START_ROUTINE GetLoadLibraryWAddress();
	PTHREAD_START_ROUTINE GetFreeLibraryAddress();

	bool CreateProccessSnapshot();
	bool IsDllLoadedToTheProcess(MODULEENTRY32W& entry);

private:
	DWORD mProcessId;
	std::wstring mDllPath;

	HANDLE mProcessSnapshot;
	HANDLE mTargetProcess;
	HANDLE mRemoteThread;
	LPWSTR mRemoteBuff;
	int mRemoteBuffSize;
};

