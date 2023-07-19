// Practice D3D11

#include "resource.h"
#include "WindowProcess.h"
#include "PracticeD3D12.h"

int MyApp::Run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    if (FAILED(InitWithCmd(lpCmdLine)))
        return E_FAIL;

    if (FAILED(InitWindow(hInstance, nCmdShow)))
        return E_FAIL;

    if (FAILED(InitDevice()))
    {
        CleanupApp();
        return E_FAIL;
    }

    if (FAILED(InitShader()))
    {
        CleanupApp();
        return E_FAIL;
    }

    if (FAILED(InitGeometry()))
    {
        CleanupApp();
        return E_FAIL;
    }

    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            InputProcess();
            UpdateProcess();
            RenderProcess();
        }
    }

    CleanupApp();

    return (int)msg.wParam;
}

HRESULT MyApp::InitWithCmd(LPWSTR lpCmdLine)
{
    return S_OK;
}

HRESULT MyApp::InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    LoadStringW(hInstance, IDS_APP_TITLE, _szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PRACTICEDIRECT3D12, _szWindowClass, MAX_LOADSTRING);

    // 윈도우 클래스를 등록합니다.
    WNDCLASSEXW wcex;
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WindowProcess;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PRACTICEDIRECT3D12));
    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName  = MAKEINTRESOURCEW(IDC_PRACTICEDIRECT3D12);
    wcex.lpszClassName = _szWindowClass;
    wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    if (!RegisterClassExW(&wcex))
        return E_FAIL;

    RECT rc = { 0, 0, 800, 600 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    _hWnd = CreateWindowW(_szWindowClass, _szTitle,
                            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                            CW_USEDEFAULT, CW_USEDEFAULT,           // x, y
                            rc.right - rc.left, rc.bottom - rc.top  // w, h
                            , nullptr, nullptr, hInstance, nullptr);

    if (!_hWnd)
        return E_FAIL;

    ShowWindow(_hWnd, nCmdShow);
    UpdateWindow(_hWnd);

    _hInstance = hInstance;

    return S_OK;
}

HRESULT MyApp::InitDevice()
{
    HRESULT hr = E_FAIL;

    return S_OK;
}

HRESULT MyApp::InitShader()
{
    HRESULT hr = E_FAIL;

    return S_OK;
}

HRESULT MyApp::InitGeometry()
{
    HRESULT hr = E_FAIL;

    return S_OK;
}

void MyApp::InputProcess()
{
}

void MyApp::UpdateProcess()
{
}

void MyApp::RenderProcess()
{
}

void MyApp::CleanupApp()
{
    _hInstance = NULL;
    _hWnd      = NULL;
}
