#pragma once

#include <iostream>

class CConsole
{
public:
	void Init(const char* title, bool input, bool output);
	void Close();
	void Print(const char* fmt, ...);

private:
	FILE* pFile = nullptr;
};

namespace Global { inline CConsole Console; }