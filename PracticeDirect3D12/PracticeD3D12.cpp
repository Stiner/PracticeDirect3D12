// Practice D3D12

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
    DebugConsole::Initialize();

    return S_OK;
}

HRESULT MyApp::InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    LoadStringW(hInstance, IDS_APP_TITLE, _szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PRACTICEDIRECT3D12, _szWindowClass, MAX_LOADSTRING);

    // 윈도우 클래스를 등록합니다.
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProcess;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PRACTICEDIRECT3D12));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_PRACTICEDIRECT3D12);
    wcex.lpszClassName = _szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    if (!RegisterClassExW(&wcex))
        return E_FAIL;

    RECT rc = { 0, 0, ScreenWidth, ScreenHeight };
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
    UINT dxgiFactoryFlags(0);
    HRESULT hr(0);

#if defined(DEBUG) || defined(_DEBUG)
    {
        ID3D12Debug* pDebugController(nullptr);
        hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController));
        if (FAILED(hr))
        {
            SAFE_RELEASE(pDebugController);
            return E_FAIL;
        }

        pDebugController->EnableDebugLayer();
        pDebugController->Release();

        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    IDXGIFactory4* pDXGIFactory4(nullptr);
    hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&pDXGIFactory4));
    if (FAILED(hr))
    {
        SAFE_RELEASE(pDXGIFactory4);
        goto END;
    }

    {
        hr = pDXGIFactory4->MakeWindowAssociation(_hWnd, DXGI_MWA_NO_ALT_ENTER);
        if (FAILED(hr))
        {
            SAFE_RELEASE(pDXGIFactory4);
            goto END;
        }
    }

    {
        hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_pD3DDevice));
        if (FAILED(hr))
        {
            IDXGIAdapter4* pDXGIAdapter4(nullptr);
            pDXGIFactory4->EnumWarpAdapter(IID_PPV_ARGS(&pDXGIAdapter4));

            hr = D3D12CreateDevice(pDXGIAdapter4, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_pD3DDevice));
            if (FAILED(hr))
            {
                SAFE_RELEASE(pDXGIAdapter4);
                goto END;
            }

            SAFE_RELEASE(pDXGIAdapter4);
        }
    }

    {
        D3D12_COMMAND_QUEUE_DESC queueDesc({});
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type  = D3D12_COMMAND_LIST_TYPE_DIRECT;

        hr = _pD3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_pCommandQueue));
        if (FAILED(hr))
            goto END;
    }

    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc({});
        swapChainDesc.BufferCount      = BACKBUFFER_COUNT;
        swapChainDesc.Width            = ScreenWidth;
        swapChainDesc.Height           = ScreenHeight;
        swapChainDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        IDXGISwapChain1* pSwapChain1(nullptr);
        hr = pDXGIFactory4->CreateSwapChainForHwnd(_pCommandQueue, _hWnd, &swapChainDesc, nullptr, nullptr, &pSwapChain1);
        if (FAILED(hr))
        {
            SAFE_RELEASE(pSwapChain1);
            goto END;
        }

        hr = pSwapChain1->QueryInterface(IID_PPV_ARGS(&_pSwapChain));
        if (FAILED(hr))
        {
            SAFE_RELEASE(pSwapChain1);
            goto END;
        }

        SAFE_RELEASE(pSwapChain1);

        _currentBackBufferIndex = _pSwapChain->GetCurrentBackBufferIndex();
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc({});
        rtvHeapDesc.NumDescriptors = BACKBUFFER_COUNT;
        rtvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        hr = _pD3DDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_pRTVDescriptorHeap));
        if (FAILED(hr))
            goto END;

        _RTVDescriptorSize = _pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    {
        D3D12_CPU_DESCRIPTOR_HANDLE handleRTVDescriptor = _pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT32 i = 0; i < BACKBUFFER_COUNT; i++)
        {
            _pSwapChain->GetBuffer(i, IID_PPV_ARGS(&_pArrRenderTargets[i]));
            _pD3DDevice->CreateRenderTargetView(_pArrRenderTargets[i], nullptr, handleRTVDescriptor);

            handleRTVDescriptor.ptr = SIZE_T(INT64(handleRTVDescriptor.ptr) + INT64(1) * _RTVDescriptorSize);
        }
    }

    {
        hr = _pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_pCommandAllocator));
        if (FAILED(hr))
            goto END;
    }

END:
    SAFE_RELEASE(pDXGIFactory4);

    return hr;
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
    _hWnd = NULL;

    SAFE_RELEASE(_pCommandAllocator);
    SAFE_RELEASE(_pRTVDescriptorHeap);
    SAFE_RELEASE(_pCommandQueue);

    for (int i = 0; i < BACKBUFFER_COUNT; i++)
    {
        SAFE_RELEASE(_pArrRenderTargets[i]);
    }

    SAFE_RELEASE(_pSwapChain);
    SAFE_RELEASE(_pD3DDevice);

    DebugConsole::Release();
}
