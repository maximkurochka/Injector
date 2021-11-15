// Injector.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
// Разработать DLL Injector использующий CreateRemoteThread. протестировать на "Hello World" динамической библиотеке

#include <iostream>
#include <string>
#include "DllInjector.h"

int main()
{
	DWORD processId;
	std::wstring dllPath;

	std::cout << "Input the process id for injection: ";
	std::cin >> processId;
	std::cin.get();

	std::cout << "Input the path to dll which will be injected: ";
	getline(std::wcin, dllPath);

	DllInjector injector(processId, dllPath);
	if (injector.Inject())
	{
		std::cout << "Dll is injected" << std::endl;
	}
	else
	{
		std::cout << "Dll is not injected" << std::endl;
		return 0;
	}

	if (injector.Eject())
	{
		std::cout << "Dll is ejected" << std::endl;
	}
	else
	{
		std::cout << "Dll is not ejected" << std::endl;
	}

	return 0;
}
