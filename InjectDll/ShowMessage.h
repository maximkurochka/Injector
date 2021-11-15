#pragma once

#ifdef SHOWMESSAGE_EXPORTS
#define SHOWMESSAGE_API __declspec(dllimport)
#else
#define SHOWMESSAGE_API __declspec(dllexport)
#endif

extern "C" SHOWMESSAGE_API void ShowMessage(const char* title, const char* text);