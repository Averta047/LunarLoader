#include "CConsole.h"

#include <Windows.h>
#include <iostream>

void CConsole::Init(const char* title, bool input, bool output)
{
	AllocConsole();

    if (output) {
        freopen_s(&pFile, "CONOUT$", "w", stdout);
    }

    if (input) {
        freopen_s(&pFile, "CONIN$", "r", stdin);
    }

	SetConsoleTitle(title);
}

void CConsole::Close()
{
    fclose(pFile);
    FreeConsole();
}

void CConsole::Print(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char buffer[256];
    vsprintf_s(buffer, format, args);
    std::string message = buffer;

    printf("%s", message.c_str());
    va_end(args);
    printf("\n");
}