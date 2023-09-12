#pragma once

void OutputDebugMessage(LPCTSTR pszStr, ...);

class DebugConsole
{
public:
    static void Initialize();
    static void Release();

    static void Print(LPCTSTR msg, ...);
};