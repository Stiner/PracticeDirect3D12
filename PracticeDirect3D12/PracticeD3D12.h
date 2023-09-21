// Practice D3D12

#pragma once

#include "framework.h"
#include "d3d12.h"
#include "dxgi1_6.h"
#include "d3dcompiler.h"
#include "DirectXMath.h"

#define SAFE_RELEASE(x) { if (x) { x->Release(); x = nullptr; } }
#define SAFE_DELETE(x)  { if (x) { delete x; } }

class MyApp
{
public:
    enum Const
    {
        MAX_LOADSTRING = 100,
        BACKBUFFER_COUNT = 2,
    };

protected:
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

public:
    HRESULT Run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow);

protected:
    HRESULT InitWithCmd(LPWSTR lpCmdLine);
    HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
    HRESULT InitDevice();
    HRESULT InitShader();
    HRESULT InitGeometry();

    void UpdateProcess(double delta);
    HRESULT RenderProcess();

    void CleanupApp();

protected:
    INT32 _screenWidth;
    INT32 _screenHeight;
    float _aspectRatio;

    TCHAR _szTitle[Const::MAX_LOADSTRING];
    TCHAR _szWindowClass[Const::MAX_LOADSTRING];

    HINSTANCE _hInstance;
    HWND      _hWnd;

    IDXGISwapChain3* _pSwapChain = nullptr;

    ID3D12Device5*             _pD3DDevice                          = nullptr;
    ID3D12Resource*            _pArrRenderTargets[BACKBUFFER_COUNT] = {};
    ID3D12CommandQueue*        _pCommandQueue                       = nullptr;
    ID3D12DescriptorHeap*      _pRTVDescriptorHeap                  = nullptr; // RTV : Render Target View
    ID3D12CommandAllocator*    _pCommandAllocator                   = nullptr;
    ID3D12GraphicsCommandList* _pCommandList                        = nullptr;
    ID3D12PipelineState*       _pPipelineState                      = nullptr;
    ID3D12RootSignature*       _pRootSignature                      = nullptr;

    D3D12_VIEWPORT _viewport;
    D3D12_RECT     _scissorRect;

    ID3D12Resource*            _pVertexBuffer                       = nullptr;
    D3D12_VERTEX_BUFFER_VIEW   _vertexBufferView;

    HANDLE       _handleFenceEvent;
    ID3D12Fence* _pFence = nullptr;
    UINT64       _fenceValue;

    UINT32 _currentBackBufferIndex;
    UINT32 _RTVDescriptorSize;
};
