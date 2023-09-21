// Practice D3D12

#include "resource.h"
#include "WindowProcess.h"
#include "PracticeD3D12.h"
#include "SuperSimpleTime.h"
#include "DebugUtil.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

HRESULT MyApp::Run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    HRESULT hr(0);

    if (FAILED(hr = InitWithCmd(lpCmdLine)))
        return E_FAIL;

    if (FAILED(hr = InitWindow(hInstance, nCmdShow)))
        return E_FAIL;

    if (FAILED(hr = InitDevice()))
        goto END;

    if (FAILED(hr = InitShader()))
        goto END;

    if (FAILED(hr = InitGeometry()))
        goto END;

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
            UpdateProcess(SuperSimpleTime::GetDeltaTime());

            if (FAILED(hr = RenderProcess()))
                goto END;
        }
    }

    hr = static_cast<HRESULT>(msg.wParam);

END:
    CleanupApp();

    return hr;
}

HRESULT MyApp::InitWithCmd(LPWSTR lpCmdLine)
{
    DebugConsole::Initialize();
    SuperSimpleTime::Initialize();

    ZeroMemory(_szTitle,       Const::MAX_LOADSTRING);
    ZeroMemory(_szWindowClass, Const::MAX_LOADSTRING);

    _hInstance              = 0;
    _hWnd                   = 0;

    _screenWidth            = 800;
    _screenHeight           = 600;
    _aspectRatio            = static_cast<float>(_screenWidth) / static_cast<float>(_screenHeight);

    _vertexBufferView       = {};

    _handleFenceEvent       = 0;
    _fenceValue             = 0;

    _currentBackBufferIndex = 0;
    _RTVDescriptorSize      = 0;

    _viewport.TopLeftX      = 0;
    _viewport.TopLeftY      = 0;
    _viewport.Width         = static_cast<float>(_screenWidth);
    _viewport.Height        = static_cast<float>(_screenHeight);
    _viewport.MinDepth      = D3D12_MIN_DEPTH;
    _viewport.MaxDepth      = D3D12_MAX_DEPTH;

    _scissorRect.left       = 0;
    _scissorRect.top        = 0;
    _scissorRect.right      = static_cast<LONG>(_screenWidth);
    _scissorRect.bottom     = static_cast<LONG>(_screenHeight);

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

    RECT rc = { 0, 0, _screenWidth, _screenHeight };
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
        swapChainDesc.Width            = _screenWidth;
        swapChainDesc.Height           = _screenHeight;
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
    HRESULT hr(0);

    {
        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc({});
        rootSignatureDesc.NumParameters     = 0;
        rootSignatureDesc.pParameters       = nullptr;
        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.pStaticSamplers   = nullptr;
        rootSignatureDesc.Flags             = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ID3DBlob* signature(nullptr);
        ID3DBlob* error(nullptr);

        hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        if (FAILED(hr))
        {
            SAFE_RELEASE(signature);
            SAFE_RELEASE(error);
            goto END;
        }

        hr = _pD3DDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_pRootSignature));
        if (FAILED(hr))
        {
            SAFE_RELEASE(signature);
            SAFE_RELEASE(error);
            return E_FAIL;
        }
    }

    {
        ID3DBlob* vertexShader(nullptr);
        ID3DBlob* pixelShader(nullptr);

#if defined(_DEBUG)
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        hr = D3DCompileFromFile(TEXT("shaders.hlsl"), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr);
        if (FAILED(hr))
        {
            SAFE_RELEASE(vertexShader);
            goto END;
        }

        hr = D3DCompileFromFile(TEXT("shaders.hlsl"), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr);
        if (FAILED(hr))
        {
            SAFE_RELEASE(pixelShader);
            goto END;
        }

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        D3D12_SHADER_BYTECODE vertexShaderByteCode({});
        vertexShaderByteCode.pShaderBytecode = vertexShader->GetBufferPointer();
        vertexShaderByteCode.BytecodeLength  = vertexShader->GetBufferSize();

        D3D12_SHADER_BYTECODE pixelShaderByteCode({});
        pixelShaderByteCode.pShaderBytecode = pixelShader->GetBufferPointer();
        pixelShaderByteCode.BytecodeLength  = pixelShader->GetBufferSize();

        D3D12_RASTERIZER_DESC rasterizerState({});
        rasterizerState.FillMode              = D3D12_FILL_MODE_SOLID;
        rasterizerState.CullMode              = D3D12_CULL_MODE_BACK;
        rasterizerState.FrontCounterClockwise = FALSE;
        rasterizerState.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
        rasterizerState.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        rasterizerState.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        rasterizerState.DepthClipEnable       = TRUE;
        rasterizerState.MultisampleEnable     = FALSE;
        rasterizerState.AntialiasedLineEnable = FALSE;
        rasterizerState.ForcedSampleCount     = 0;
        rasterizerState.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        D3D12_BLEND_DESC blendState({});
        blendState.AlphaToCoverageEnable = FALSE;
        blendState.IndependentBlendEnable = FALSE;
        {
            D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc({});
            defaultRenderTargetBlendDesc.BlendEnable           = FALSE;
            defaultRenderTargetBlendDesc.LogicOpEnable         = FALSE;
            defaultRenderTargetBlendDesc.SrcBlend              = D3D12_BLEND_ONE;
            defaultRenderTargetBlendDesc.DestBlend             = D3D12_BLEND_ZERO;
            defaultRenderTargetBlendDesc.BlendOp               = D3D12_BLEND_OP_ADD;
            defaultRenderTargetBlendDesc.SrcBlendAlpha         = D3D12_BLEND_ONE;
            defaultRenderTargetBlendDesc.DestBlendAlpha        = D3D12_BLEND_ZERO;
            defaultRenderTargetBlendDesc.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
            defaultRenderTargetBlendDesc.LogicOp               = D3D12_LOGIC_OP_NOOP;
            defaultRenderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
            {
                blendState.RenderTarget[i] = defaultRenderTargetBlendDesc;
            }
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout                     = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature                  = _pRootSignature;
        psoDesc.VS                              = vertexShaderByteCode;
        psoDesc.PS                              = pixelShaderByteCode;
        psoDesc.RasterizerState                 = rasterizerState;
        psoDesc.BlendState                      = blendState;
        psoDesc.DepthStencilState.DepthEnable   = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask                      = UINT_MAX;
        psoDesc.PrimitiveTopologyType           = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets                = 1;
        psoDesc.RTVFormats[0]                   = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count                = 1;

        hr = _pD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_pPipelineState));
        if (FAILED(hr))
            goto END;
    }

    hr = _pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _pCommandAllocator, _pPipelineState, IID_PPV_ARGS(&_pCommandList));
    if (FAILED(hr))
        goto END;

    hr = _pCommandList->Close();
    if (FAILED(hr))
        goto END;

END:
    return hr;
}

HRESULT MyApp::InitGeometry()
{
    HRESULT hr(0);

    {
        Vertex triangleVertices[] =
        {
            { {  0.00f,  0.25f * _aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { {  0.25f, -0.25f * _aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f * _aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        const UINT vertexBufferSize = sizeof(triangleVertices);

        D3D12_HEAP_PROPERTIES heapProp({});
        heapProp.Type                 = D3D12_HEAP_TYPE_UPLOAD;
        heapProp.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProp.CreationNodeMask     = 1;
        heapProp.VisibleNodeMask      = 1;

        D3D12_RESOURCE_DESC resourceDesc({});
        resourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment          = 0;
        resourceDesc.Width              = vertexBufferSize;
        resourceDesc.Height             = 1;
        resourceDesc.DepthOrArraySize   = 1;
        resourceDesc.MipLevels          = 1;
        resourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count   = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        hr = _pD3DDevice->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_pVertexBuffer));
        if (FAILED(hr))
            goto END;

        UINT8* pVertexDataBegin;
        D3D12_RANGE readRange = { 0, 0 };
        hr = _pVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
        if (FAILED(hr))
            goto END;

        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        _pVertexBuffer->Unmap(0, nullptr);

        _vertexBufferView.BufferLocation = _pVertexBuffer->GetGPUVirtualAddress();
        _vertexBufferView.StrideInBytes  = sizeof(Vertex);
        _vertexBufferView.SizeInBytes    = vertexBufferSize;
    }

    {
        hr = _pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_pFence));
        _fenceValue = 1;

        _handleFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!_handleFenceEvent)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto END;
        }

        {
            const UINT64 fence = _fenceValue;
            hr = _pCommandQueue->Signal(_pFence, fence);
            if (FAILED(hr))
                goto END;

            _fenceValue++;

            if (_pFence->GetCompletedValue() < fence)
            {
                hr = _pFence->SetEventOnCompletion(fence, _handleFenceEvent);
                if (FAILED(hr))
                    goto END;

                WaitForSingleObject(_handleFenceEvent, INFINITE);
            }

            _currentBackBufferIndex = _pSwapChain->GetCurrentBackBufferIndex();
        }
    }

END:
    return hr;
}

void MyApp::UpdateProcess(double delta)
{
    SuperSimpleTime::Update();
}

HRESULT MyApp::RenderProcess()
{
    HRESULT hr(0);

    {
        hr = _pCommandAllocator->Reset();
        if (FAILED(hr))
            goto END;

        hr = _pCommandList->Reset(_pCommandAllocator, _pPipelineState);
        if (FAILED(hr))
            goto END;

        _pCommandList->SetGraphicsRootSignature(_pRootSignature);
        _pCommandList->RSSetViewports(1, &_viewport);
        _pCommandList->RSSetScissorRects(1, &_scissorRect);

        {
            D3D12_RESOURCE_BARRIER barrier({});
            barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource   = _pArrRenderTargets[_currentBackBufferIndex];
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            _pCommandList->ResourceBarrier(1, &barrier);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE handleRTVDescriptor({});
        handleRTVDescriptor.ptr = SIZE_T(INT64(_pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr) + (INT64(_currentBackBufferIndex) * _RTVDescriptorSize));

        _pCommandList->OMSetRenderTargets(1, &handleRTVDescriptor, FALSE, nullptr);

        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        _pCommandList->ClearRenderTargetView(handleRTVDescriptor, clearColor, 0, nullptr);
        _pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        _pCommandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
        _pCommandList->DrawInstanced(3, 1, 0, 0);

        {
            D3D12_RESOURCE_BARRIER barrier({});
            barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource   = _pArrRenderTargets[_currentBackBufferIndex];
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            _pCommandList->ResourceBarrier(1, &barrier);
        }

        hr = _pCommandList->Close();
        if (FAILED(hr))
            goto END;
    }

    {
        ID3D12CommandList* pArrCommandList[] = { _pCommandList };
        _pCommandQueue->ExecuteCommandLists(_countof(pArrCommandList), pArrCommandList);
    }

    hr = _pSwapChain->Present(1, 0);
    if (FAILED(hr))
        goto END;

    {
        const UINT64 fence = _fenceValue;
        hr = _pCommandQueue->Signal(_pFence, fence);
        if (FAILED(hr))
            goto END;

        _fenceValue++;

        if (_pFence->GetCompletedValue() < fence)
        {
            hr = _pFence->SetEventOnCompletion(fence, _handleFenceEvent);
            if (FAILED(hr))
                goto END;

            WaitForSingleObject(_handleFenceEvent, INFINITE);
        }

        _currentBackBufferIndex = _pSwapChain->GetCurrentBackBufferIndex();
    }

END:
    return hr;
}

void MyApp::CleanupApp()
{
    _hInstance = NULL;
    _hWnd = NULL;

    SAFE_RELEASE(_pFence);
    SAFE_RELEASE(_pCommandList);
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
