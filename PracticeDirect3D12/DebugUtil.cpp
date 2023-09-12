#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "DebugUtil.h"

void OutputDebugMessage(LPCTSTR pszStr, ...)
{
#if _DEBUG
    TCHAR szMsg[256];
    va_list args;
    va_start(args, pszStr);
    _vstprintf_s(szMsg, 256, pszStr, args);
    OutputDebugString(szMsg);
#endif
}

static FILE* g_pFile;

void DebugConsole::Initialize()
{
    AllocConsole();
    _tfreopen_s(&g_pFile, TEXT("CONOUT$"), TEXT("w"), stdout);
}

void DebugConsole::Release()
{
    fclose(g_pFile);
    FreeConsole();
}

void DebugConsole::Print(LPCTSTR pszStr, ...)
{
#if _DEBUG
    TCHAR szMsg[256];
    va_list args;
    va_start(args, pszStr);
    _vstprintf_s(szMsg, 256, pszStr, args);

    _tprintf_s(szMsg);
#endif
}
