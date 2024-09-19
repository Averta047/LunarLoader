#pragma once

#include "Math.h"

class CUtil_Pattern
{
public:
	DWORD Find(const char* const szModule, const char* const szPattern);

private:
	DWORD FindPattern(const DWORD dwAddress, const DWORD dwLen, const char* const szPattern);
};

namespace Util { inline CUtil_Pattern Pattern; }