// Practice D3D12

#include "PracticeD3D12.h"

int APIENTRY wWinMain(_In_     HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_     LPWSTR    lpCmdLine,
                      _In_     int       nCmdShow)
{
    MyApp* pApp = new MyApp();
    int r = pApp->Run(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    delete pApp;

    return r;
}
