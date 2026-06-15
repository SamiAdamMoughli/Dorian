#pragma once
#include <windows.h>
#include "Common.hpp"

class SectionManager
{
public:
    static HANDLE CreateImageSection(HANDLE fileHandle);
};
