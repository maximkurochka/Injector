#include "pch.h"
#include "ShowMessage.h"

SHOWMESSAGE_API void ShowMessage(const char* title, const char* text)
{
	MessageBoxA(NULL, text, title, MB_OK | MB_ICONINFORMATION);
}
