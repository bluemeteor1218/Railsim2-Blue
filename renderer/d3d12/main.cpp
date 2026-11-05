#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <array>
#include <cwchar>
namespace {

using Microsoft::WRL::ComPtr;

constexpr UINT kFrameCount = 2;
constexpr wchar_t kWindowClass[] = L"RailSim2D3D12Window";
constexpr wchar_t kWindowTitle[] = L"RailSim2 - Direct3D 12 renderer";

bool ShowHResultError(HWND owner, const wchar_t* operation, HRESULT result)
{
    wchar_t message[256]{};
    swprintf_s(message, L"%s failed (HRESULT 0x%08X).", operation,
        static_cast<unsigned int>(result));
    MessageBoxW(owner, message, L"RailSim2 renderer error", MB_OK | MB_ICONERROR);
    return false;
}

bool ShowWin32Error(HWND owner, const wchar_t* operation, DWORD error)
{
    wchar_t message[256]{};
    swprintf_s(message, L"%s failed (Windows error %lu).", operation,
        static_cast<unsigned long>(error));
    MessageBoxW(owner, message, L"RailSim2 renderer error", MB_OK | MB_ICONERROR);
    return false;
}

class Renderer {
public:
    bool Initialize(HWND window)
    {
        window_ = window;

        HRESULT result = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory_));
        if (FAILED(result)) {
            return ShowHResultError(window_, L"CreateDXGIFactory2", result);
        }

        result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&device_));
        if (FAILED(result)) {
            return ShowHResultError(window_, L"D3D12CreateDevice", result);
        }

        D3D12_COMMAND_QUEUE_DESC queueDescription{};
        queueDescription.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        result = device_->CreateCommandQueue(&queueDescription,
            IID_PPV_ARGS(&commandQueue_));
        if (FAILED(result)) {
            return ShowHResultError(window_, L"CreateCommandQueue", result);
        }

        D3D12_DESCRIPTOR_HEAP_DESC heapDescription{};
        heapDescription.NumDescriptors = kFrameCount;
        heapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        result = device_->CreateDescriptorHeap(&heapDescription,
            IID_PPV_ARGS(&rtvHeap_));
        if (FAILED(result)) {
            return ShowHResultError(window_, L"CreateDescriptorHeap", result);
        }
        rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        for (auto& allocator : commandAllocators_) {
            result = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&allocator));
            if (FAILED(result)) {
                return ShowHResultError(window_, L"CreateCommandAllocator", result);
            }
        }

        result = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            commandAllocators_[0].Get(), nullptr, IID_PPV_ARGS(&commandList_));
        if (FAILED(result)) {
            return ShowHResultError(window_, L"CreateCommandList", result);
        }
        result = commandList_->Close();
        if (FAILED(result)) {
            return ShowHResultError(window_, L"Close command list", result);
        }

        DXGI_SWAP_CHAIN_DESC1 swapChainDescription{};
        RECT clientRect{};
        if (!GetClientRect(window_, &clientRect)) {
            return ShowWin32Error(window_, L"GetClientRect", GetLastError());
        }
        swapChainDescription.Width = static_cast<UINT>(clientRect.right - clientRect.left);
        swapChainDescription.Height = static_cast<UINT>(clientRect.bottom - clientRect.top);
        swapChainDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDescription.SampleDesc.Count = 1;
        swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDescription.BufferCount = kFrameCount;
        swapChainDescription.Scaling = DXGI_SCALING_STRETCH;
        swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        ComPtr<IDXGISwapChain1> swapChain;
        result = factory_->CreateSwapChainForHwnd(commandQueue_.Get(), window_,
            &swapChainDescription, nullptr, nullptr, &swapChain);
        if (FAILED(result)) {
            return ShowHResultError(window_, L"CreateSwapChainForHwnd", result);
        }

        result = factory_->MakeWindowAssociation(window_, DXGI_MWA_NO_ALT_ENTER);
        if (FAILED(result)) {
            return ShowHResultError(window_, L"MakeWindowAssociation", result);
        }

        result = swapChain.As(&swapChain_);
        if (FAILED(result)) {
            return ShowHResultError(window_, L"Query IDXGISwapChain3", result);
        }

        fenceEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (fenceEvent_ == nullptr) {
            return ShowWin32Error(window_, L"CreateEventW", GetLastError());
        }

        result = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
        if (FAILED(result)) {
            return ShowHResultError(window_, L"CreateFence", result);
        }

        initialized_ = true;
        return CreateRenderTargets();
    }

    ~Renderer()
    {
        if (commandQueue_ && fence_ && fenceEvent_ != nullptr) {
            WaitForGpu();
        }
        if (fenceEvent_ != nullptr) {
            CloseHandle(fenceEvent_);
        }
    }

    bool Render()
    {
        if (!initialized_ || width_ == 0 || height_ == 0) {
            return true;
        }

        frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
        const UINT64 frameFenceValue = frameFenceValues_[frameIndex_];
        if (fence_->GetCompletedValue() < frameFenceValue) {
            HRESULT result = fence_->SetEventOnCompletion(frameFenceValue, fenceEvent_);
            if (FAILED(result)) {
                return ShowHResultError(window_, L"SetEventOnCompletion", result);
            }
            if (WaitForSingleObject(fenceEvent_, INFINITE) != WAIT_OBJECT_0) {
                return ShowWin32Error(window_, L"WaitForSingleObject", GetLastError());
            }
        }

        HRESULT result = commandAllocators_[frameIndex_]->Reset();
        if (FAILED(result)) {
            return ShowHResultError(window_, L"Reset command allocator", result);
        }
        result = commandList_->Reset(commandAllocators_[frameIndex_].Get(), nullptr);
        if (FAILED(result)) {
            return ShowHResultError(window_, L"Reset command list", result);
        }

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = renderTargets_[frameIndex_].Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        commandList_->ResourceBarrier(1, &barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE renderTarget = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
        renderTarget.ptr += static_cast<SIZE_T>(frameIndex_) * rtvDescriptorSize_;
        commandList_->OMSetRenderTargets(1, &renderTarget, FALSE, nullptr);
        constexpr float clearColor[] = {0.08f, 0.12f, 0.18f, 1.0f};
        commandList_->ClearRenderTargetView(renderTarget, clearColor, 0, nullptr);

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        commandList_->ResourceBarrier(1, &barrier);

        result = commandList_->Close();
        if (FAILED(result)) {
            return ShowHResultError(window_, L"Close command list", result);
        }

        ID3D12CommandList* commandLists[] = {commandList_.Get()};
        commandQueue_->ExecuteCommandLists(1, commandLists);

        result = swapChain_->Present(1, 0);
        if (FAILED(result)) {
            return ShowHResultError(window_, L"Present", result);
        }

        const UINT64 nextFenceValue = nextFenceValue_++;
        result = commandQueue_->Signal(fence_.Get(), nextFenceValue);
        if (FAILED(result)) {
            return ShowHResultError(window_, L"Signal fence", result);
        }
        frameFenceValues_[frameIndex_] = nextFenceValue;
        return true;
    }

    bool Resize(UINT width, UINT height)
    {
        if (!initialized_ || width == 0 || height == 0 ||
            (width == width_ && height == height_)) {
            return true;
        }
        if (!WaitForGpu()) {
            return false;
        }

        for (auto& target : renderTargets_) {
            target.Reset();
        }

        HRESULT result = swapChain_->ResizeBuffers(kFrameCount, width, height,
            DXGI_FORMAT_R8G8B8A8_UNORM, 0);
        if (FAILED(result)) {
            return ShowHResultError(window_, L"ResizeBuffers", result);
        }
        width_ = width;
        height_ = height;
        return CreateRenderTargets();
    }

private:
    bool CreateRenderTargets()
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
        for (UINT index = 0; index < kFrameCount; ++index) {
            HRESULT result = swapChain_->GetBuffer(index, IID_PPV_ARGS(&renderTargets_[index]));
            if (FAILED(result)) {
                return ShowHResultError(window_, L"GetBuffer", result);
            }
            device_->CreateRenderTargetView(renderTargets_[index].Get(), nullptr, handle);
            handle.ptr += rtvDescriptorSize_;
        }

        DXGI_SWAP_CHAIN_DESC1 description{};
        HRESULT result = swapChain_->GetDesc1(&description);
        if (FAILED(result)) {
            return ShowHResultError(window_, L"GetDesc1", result);
        }
        width_ = description.Width;
        height_ = description.Height;
        return true;
    }

    bool WaitForGpu()
    {
        const UINT64 value = nextFenceValue_++;
        HRESULT result = commandQueue_->Signal(fence_.Get(), value);
        if (FAILED(result)) {
            return ShowHResultError(window_, L"Signal fence", result);
        }
        result = fence_->SetEventOnCompletion(value, fenceEvent_);
        if (FAILED(result)) {
            return ShowHResultError(window_, L"SetEventOnCompletion", result);
        }
        if (WaitForSingleObject(fenceEvent_, INFINITE) != WAIT_OBJECT_0) {
            return ShowWin32Error(window_, L"WaitForSingleObject", GetLastError());
        }
        return true;
    }

    HWND window_ = nullptr;
    HANDLE fenceEvent_ = nullptr;
    bool initialized_ = false;
    UINT width_ = 0;
    UINT height_ = 0;
    UINT frameIndex_ = 0;
    UINT rtvDescriptorSize_ = 0;
    UINT64 nextFenceValue_ = 1;
    std::array<UINT64, kFrameCount> frameFenceValues_{};
    std::array<ComPtr<ID3D12CommandAllocator>, kFrameCount> commandAllocators_;
    std::array<ComPtr<ID3D12Resource>, kFrameCount> renderTargets_;
    ComPtr<IDXGIFactory4> factory_;
    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12CommandQueue> commandQueue_;
    ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    ComPtr<ID3D12GraphicsCommandList> commandList_;
    ComPtr<IDXGISwapChain3> swapChain_;
    ComPtr<ID3D12Fence> fence_;
};

Renderer* g_renderer = nullptr;

LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_SIZE:
        if (g_renderer != nullptr && wParam != SIZE_MINIMIZED) {
            const UINT width = LOWORD(lParam);
            const UINT height = HIWORD(lParam);
            if (!g_renderer->Resize(width, height)) {
                PostQuitMessage(1);
            }
        }
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT paint{};
        BeginPaint(window, &paint);
        EndPaint(window, &paint);
        if (g_renderer != nullptr && !g_renderer->Render()) {
            PostQuitMessage(1);
        }
        return 0;
    }
    case WM_DPICHANGED:
    {
        const auto* suggestedRect = reinterpret_cast<const RECT*>(lParam);
        if (!SetWindowPos(window, nullptr, suggestedRect->left, suggestedRect->top,
                suggestedRect->right - suggestedRect->left,
                suggestedRect->bottom - suggestedRect->top,
                SWP_NOZORDER | SWP_NOACTIVATE)) {
            ShowWin32Error(window, L"SetWindowPos", GetLastError());
            PostQuitMessage(1);
        }
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        ShowWin32Error(nullptr, L"SetProcessDpiAwarenessContext", GetLastError());
        return 1;
    }

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = kWindowClass;

    if (RegisterClassExW(&windowClass) == 0) {
        ShowWin32Error(nullptr, L"RegisterClassExW", GetLastError());
        return 1;
    }

    RECT windowRect{0, 0, 1280, 720};
    if (!AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE)) {
        ShowWin32Error(nullptr, L"AdjustWindowRect", GetLastError());
        return 1;
    }

    HWND window = CreateWindowExW(0, kWindowClass, kWindowTitle,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        nullptr, nullptr, instance, nullptr);
    if (window == nullptr) {
        ShowWin32Error(nullptr, L"CreateWindowExW", GetLastError());
        return 1;
    }

    Renderer renderer;
    g_renderer = &renderer;
    if (!renderer.Initialize(window)) {
        g_renderer = nullptr;
        DestroyWindow(window);
        return 1;
    }

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};
    while (true) {
        const BOOL result = GetMessageW(&message, nullptr, 0, 0);
        if (result == 0) {
            break;
        }
        if (result == -1) {
            ShowWin32Error(window, L"GetMessageW", GetLastError());
            g_renderer = nullptr;
            return 1;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    g_renderer = nullptr;
    return static_cast<int>(message.wParam);
}
