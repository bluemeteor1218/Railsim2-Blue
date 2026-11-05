#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <array>
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <string>

#include "constant_ring.h"
#include "d3dx_math.h"
#include "font_atlas.h"
#include "pipeline.h"
#include "render_state.h"
#include "text_layout.h"
#include "texture.h"
#include "ui_renderer.h"
#include "viewport_layout.h"

namespace {

using Microsoft::WRL::ComPtr;

constexpr UINT kFrameCount = 2;
constexpr DXGI_FORMAT kDepthFormat = DXGI_FORMAT_D32_FLOAT;
// Legacy RailSim2 UI design size and base font size in virtual pixels.
constexpr float kUiWidth = 640.0f;
constexpr float kUiHeight = 480.0f;
constexpr float kUiFontSize = 14.0f;
constexpr wchar_t kWindowClass[] = L"RailSim2D3D12Window";
constexpr wchar_t kWindowTitle[] = L"RailSim2 - Direct3D 12 renderer";

struct Mesh {
    ComPtr<ID3D12Resource> vertices;
    ComPtr<ID3D12Resource> indices;
    D3D12_VERTEX_BUFFER_VIEW vertexView{};
    D3D12_INDEX_BUFFER_VIEW indexView{};
    UINT indexCount = 0;
};

// Unit cube with per-face normals. Front faces are clockwise (Direct3D, left-handed).
void BuildCube(const uint8_t color[4], std::array<rs2::Vertex, 24>* vertices, std::array<uint16_t, 36>* indices)
{
    const float normals[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (int face = 0; face < 6; ++face) {
        const D3DXVECTOR3 n(normals[face]);
        const D3DXVECTOR3 u = std::fabs(n.y) > 0.5f ? D3DXVECTOR3(1, 0, 0) : D3DXVECTOR3(0, 1, 0);
        D3DXVECTOR3 v;
        D3DXVec3Cross(&v, &u, &n);
        const float signs[4][2] = {{-1, -1}, {-1, 1}, {1, 1}, {1, -1}};
        for (int corner = 0; corner < 4; ++corner) {
            const D3DXVECTOR3 p = n + u * signs[corner][0] + v * signs[corner][1];
            rs2::Vertex& out = (*vertices)[static_cast<size_t>(face * 4 + corner)];
            out.position[0] = p.x * 0.5f;
            out.position[1] = p.y * 0.5f;
            out.position[2] = p.z * 0.5f;
            out.normal[0] = n.x;
            out.normal[1] = n.y;
            out.normal[2] = n.z;
            out.uv[0] = signs[corner][0] * 0.5f + 0.5f;
            out.uv[1] = 0.5f - signs[corner][1] * 0.5f;
            std::memcpy(out.color, color, 4);
        }
        const uint16_t base = static_cast<uint16_t>(face * 4);
        const uint16_t quad[6] = {0, 1, 2, 0, 2, 3};
        for (int i = 0; i < 6; ++i) {
            (*indices)[static_cast<size_t>(face * 6 + i)] = static_cast<uint16_t>(base + quad[i]);
        }
    }
}

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

// Looks for the shipped sample texture in the Distribution folder above the executable.
std::wstring FindSampleTexture()
{
    wchar_t module[MAX_PATH]{};
    GetModuleFileNameW(nullptr, module, MAX_PATH);
    std::wstring directory(module);
    for (int depth = 0; depth < 10; ++depth) {
        const size_t slash = directory.find_last_of(L'\\');
        if (slash == std::wstring::npos) {
            break;
        }
        directory.resize(slash);
        const std::wstring candidate =
            directory + L"\\Distribution\\en\\RailSim2\\Girder\\Default_JRN_SinglePC\\Concrete.png";
        if (GetFileAttributesW(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return candidate;
        }
    }
    return std::wstring();
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
        if (!CreateRenderTargets()) {
            return false;
        }
        return CreateScene();
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
        const D3D12_CPU_DESCRIPTOR_HANDLE depthTarget = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
        commandList_->OMSetRenderTargets(1, &renderTarget, FALSE, &depthTarget);
        constexpr float clearColor[] = {0.08f, 0.12f, 0.18f, 1.0f};
        commandList_->ClearRenderTargetView(renderTarget, clearColor, 0, nullptr);
        commandList_->ClearDepthStencilView(depthTarget, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        if (!DrawScene()) {
            return false;
        }
        DrawUi();

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
        if (!CreateRenderTargets()) {
            return false;
        }
        return RebuildFont();
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
        return CreateDepthBuffer();
    }

    bool CreateDepthBuffer()
    {
        if (!dsvHeap_) {
            D3D12_DESCRIPTOR_HEAP_DESC heapDescription{};
            heapDescription.NumDescriptors = 1;
            heapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            const HRESULT heapResult = device_->CreateDescriptorHeap(&heapDescription, IID_PPV_ARGS(&dsvHeap_));
            if (FAILED(heapResult)) {
                return ShowHResultError(window_, L"CreateDescriptorHeap (DSV)", heapResult);
            }
        }

        D3D12_HEAP_PROPERTIES heap{};
        heap.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width = width_;
        desc.Height = height_;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = kDepthFormat;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        D3D12_CLEAR_VALUE clear{};
        clear.Format = kDepthFormat;
        clear.DepthStencil.Depth = 1.0f;

        depthBuffer_.Reset();
        const HRESULT depthResult = device_->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear, IID_PPV_ARGS(&depthBuffer_));
        if (FAILED(depthResult)) {
            return ShowHResultError(window_, L"CreateCommittedResource (depth)", depthResult);
        }
        device_->CreateDepthStencilView(depthBuffer_.Get(), nullptr, dsvHeap_->GetCPUDescriptorHandleForHeapStart());
        return true;
    }

    ComPtr<ID3D12Resource> CreateBuffer(D3D12_HEAP_TYPE type, UINT64 size, D3D12_RESOURCE_STATES state)
    {
        D3D12_HEAP_PROPERTIES heap{};
        heap.Type = type;
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = size;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ComPtr<ID3D12Resource> buffer;
        const HRESULT result =
            device_->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr, IID_PPV_ARGS(&buffer));
        if (FAILED(result)) {
            ShowHResultError(window_, L"CreateCommittedResource (buffer)", result);
            return nullptr;
        }
        return buffer;
    }

    // Static geometry lives in default (GPU-local) heaps; staged through upload
    // heaps with a synchronous copy. Only call while no frame is being recorded.
    bool CreateMesh(const void* vertexData, UINT vertexBytes, UINT vertexStride, const uint16_t* indexData,
                    UINT indexCount, Mesh* mesh)
    {
        const UINT indexBytes = indexCount * sizeof(uint16_t);
        mesh->vertices = CreateBuffer(D3D12_HEAP_TYPE_DEFAULT, vertexBytes, D3D12_RESOURCE_STATE_COPY_DEST);
        mesh->indices = CreateBuffer(D3D12_HEAP_TYPE_DEFAULT, indexBytes, D3D12_RESOURCE_STATE_COPY_DEST);
        ComPtr<ID3D12Resource> vertexStage = CreateBuffer(D3D12_HEAP_TYPE_UPLOAD, vertexBytes, D3D12_RESOURCE_STATE_GENERIC_READ);
        ComPtr<ID3D12Resource> indexStage = CreateBuffer(D3D12_HEAP_TYPE_UPLOAD, indexBytes, D3D12_RESOURCE_STATE_GENERIC_READ);
        if (!mesh->vertices || !mesh->indices || !vertexStage || !indexStage) {
            return false;
        }
        const D3D12_RANGE noRead{0, 0};
        void* mapped = nullptr;
        if (FAILED(vertexStage->Map(0, &noRead, &mapped))) {
            return ShowHResultError(window_, L"Map vertex staging", E_FAIL);
        }
        std::memcpy(mapped, vertexData, vertexBytes);
        vertexStage->Unmap(0, nullptr);
        if (FAILED(indexStage->Map(0, &noRead, &mapped))) {
            return ShowHResultError(window_, L"Map index staging", E_FAIL);
        }
        std::memcpy(mapped, indexData, indexBytes);
        indexStage->Unmap(0, nullptr);

        HRESULT result = commandAllocators_[0]->Reset();
        if (SUCCEEDED(result)) {
            result = commandList_->Reset(commandAllocators_[0].Get(), nullptr);
        }
        if (FAILED(result)) {
            return ShowHResultError(window_, L"Reset command list (mesh upload)", result);
        }
        commandList_->CopyBufferRegion(mesh->vertices.Get(), 0, vertexStage.Get(), 0, vertexBytes);
        commandList_->CopyBufferRegion(mesh->indices.Get(), 0, indexStage.Get(), 0, indexBytes);
        D3D12_RESOURCE_BARRIER barriers[2]{};
        const struct { ID3D12Resource* resource; D3D12_RESOURCE_STATES after; } transitions[2] = {
            {mesh->vertices.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER},
            {mesh->indices.Get(), D3D12_RESOURCE_STATE_INDEX_BUFFER}};
        for (int i = 0; i < 2; ++i) {
            barriers[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[i].Transition.pResource = transitions[i].resource;
            barriers[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers[i].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barriers[i].Transition.StateAfter = transitions[i].after;
        }
        commandList_->ResourceBarrier(2, barriers);
        result = commandList_->Close();
        if (FAILED(result)) {
            return ShowHResultError(window_, L"Close command list (mesh upload)", result);
        }
        ID3D12CommandList* lists[] = {commandList_.Get()};
        commandQueue_->ExecuteCommandLists(1, lists);
        if (!WaitForGpu()) {
            return false;
        }

        mesh->vertexView = {mesh->vertices->GetGPUVirtualAddress(), vertexBytes, vertexStride};
        mesh->indexView = {mesh->indices->GetGPUVirtualAddress(), indexBytes, DXGI_FORMAT_R16_UINT};
        mesh->indexCount = indexCount;
        return true;
    }

    bool CreateCubeMesh(const uint8_t color[4], Mesh* mesh)
    {
        std::array<rs2::Vertex, 24> vertices;
        std::array<uint16_t, 36> indices;
        BuildCube(color, &vertices, &indices);
        return CreateMesh(vertices.data(), sizeof(vertices), sizeof(rs2::Vertex), indices.data(),
                          static_cast<UINT>(indices.size()), mesh);
    }

    bool CreateScene()
    {
        std::string error;
        if (!pipelines_.Initialize(device_.Get(), DXGI_FORMAT_R8G8B8A8_UNORM, kDepthFormat, &error)) {
            MessageBoxA(window_, error.c_str(), "RailSim2 renderer error", MB_OK | MB_ICONERROR);
            return false;
        }
        if (!constants_.Initialize(device_.Get())) {
            MessageBoxA(window_, "Failed to create the constant ring", "RailSim2 renderer error", MB_OK | MB_ICONERROR);
            return false;
        }
        const uint8_t opaque[4] = {230, 120, 40, 255};
        const uint8_t glass[4] = {80, 160, 255, 110};
        if (!CreateCubeMesh(opaque, &opaqueCube_) || !CreateCubeMesh(glass, &glassCube_)) {
            return false;
        }
        if (!CreateSampleTexture()) {
            return false;
        }
        if (!CreateUi()) {
            return false;
        }
        startTime_ = GetTickCount64();
        fpsWindowStart_ = startTime_;
        return true;
    }

    // Loads the shipped PNG (checkerboard fallback), builds mips and uploads it synchronously.
    bool CreateSampleTexture()
    {
        rs2::Image image;
        const std::wstring path = FindSampleTexture();
        if (path.empty() || !rs2::LoadImageWic(path.c_str(), &image)) {
            image = rs2::MakeCheckerboard(256, 32);
        }
        const std::vector<rs2::Image> mips = rs2::GenerateMipChain(image);

        D3D12_DESCRIPTOR_HEAP_DESC heapDescription{};
        heapDescription.NumDescriptors = 1;
        heapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        const HRESULT result = device_->CreateDescriptorHeap(&heapDescription, IID_PPV_ARGS(&srvHeap_));
        if (FAILED(result)) {
            return ShowHResultError(window_, L"CreateDescriptorHeap (SRV)", result);
        }
        if (!UploadTexture(mips, &texture_)) {
            return false;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC view{};
        view.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view.Texture2D.MipLevels = static_cast<UINT>(mips.size());
        device_->CreateShaderResourceView(texture_.Get(), &view, srvHeap_->GetCPUDescriptorHandleForHeapStart());
        return true;
    }

    // Synchronous upload; only call while no frame is being recorded.
    bool UploadTexture(const std::vector<rs2::Image>& mips, ComPtr<ID3D12Resource>* texture)
    {
        HRESULT result = commandAllocators_[0]->Reset();
        if (SUCCEEDED(result)) {
            result = commandList_->Reset(commandAllocators_[0].Get(), nullptr);
        }
        if (FAILED(result)) {
            return ShowHResultError(window_, L"Reset command list (upload)", result);
        }
        ComPtr<ID3D12Resource> uploadHeap;
        if (!rs2::CreateTextureFromMips(device_.Get(), commandList_.Get(), mips, texture, &uploadHeap)) {
            commandList_->Close();
            return ShowHResultError(window_, L"Create texture", E_FAIL);
        }
        result = commandList_->Close();
        if (FAILED(result)) {
            return ShowHResultError(window_, L"Close command list (upload)", result);
        }
        ID3D12CommandList* lists[] = {commandList_.Get()};
        commandQueue_->ExecuteCommandLists(1, lists);
        return WaitForGpu();
    }

    rs2::ViewportLayout MakeLayout() const
    {
        return rs2::ViewportLayout(kUiWidth, kUiHeight, static_cast<float>(width_), static_cast<float>(height_),
                                   rs2::LayoutMode::FixedHeight);
    }

    bool CreateUi()
    {
        std::string error;
        if (!ui_.Initialize(device_.Get(), DXGI_FORMAT_R8G8B8A8_UNORM, kDepthFormat, &error)) {
            MessageBoxA(window_, error.c_str(), "RailSim2 renderer error", MB_OK | MB_ICONERROR);
            return false;
        }
        rs2::Image white;
        white.width = white.height = 1;
        white.rgba = {255, 255, 255, 255};
        if (!UploadTexture({white}, &whiteTexture_)) {
            return false;
        }
        ui_.SetTexture(0, whiteTexture_.Get());
        return RebuildFont();
    }

    // Rasterizes the font at the physical pixel size of the current UI scale so glyphs stay 1:1 with the screen.
    bool RebuildFont()
    {
        const int pixelHeight =
            std::max(8, static_cast<int>(std::lround(kUiFontSize * MakeLayout().Scale())));
        if (pixelHeight == fontPixelHeight_) {
            return true;
        }
        rs2::Image atlas;
        rs2::GlyphTable table;
        if (!rs2::BuildFontAtlas(L"Segoe UI", pixelHeight, &atlas, &table)) {
            return true; // keep the previous atlas
        }
        ComPtr<ID3D12Resource> texture;
        if (!UploadTexture({atlas}, &texture)) {
            return false;
        }
        fontTexture_ = std::move(texture);
        glyphs_ = table;
        ui_.SetTexture(1, fontTexture_.Get());
        fontPixelHeight_ = pixelHeight;
        return true;
    }

    void DrawUi()
    {
        const rs2::ViewportLayout layout = MakeLayout();
        const float scale = layout.Scale();
        const auto snap = [scale](float v) { return std::round(v * scale) / scale; };

        const ULONGLONG now = GetTickCount64();
        ++framesSinceFpsUpdate_;
        if (now - fpsWindowStart_ >= 500) {
            fps_ = static_cast<float>(framesSinceFpsUpdate_) * 1000.0f / static_cast<float>(now - fpsWindowStart_);
            framesSinceFpsUpdate_ = 0;
            fpsWindowStart_ = now;
        }

        char text[160];
        std::snprintf(text, sizeof(text), "RailSim2  Direct3D 12\nFPS: %.0f\n%ux%u px   UI x%.2f", fps_, width_,
                      height_, scale);

        const float textScale = 1.0f / scale;
        const float margin = 8.0f;
        const rs2::TextMetrics size = rs2::LayoutText(glyphs_, text, 0, 0, textScale, {}, nullptr);

        batch_.Clear();
        batch_.SetTexture(0);
        const rs2::Rect panel{snap(margin), snap(margin), snap(size.width + 12.0f), snap(size.height + 8.0f)};
        batch_.AddSolid(panel, {0, 0, 0, 150});
        batch_.AddFrame(panel, 1.0f / scale, {120, 170, 255, 255});
        batch_.SetTexture(1);
        rs2::LayoutText(glyphs_, text, snap(panel.x + 6.0f), snap(panel.y + 4.0f), textScale, {255, 255, 255, 255},
                        &batch_);
        ui_.Draw(commandList_.Get(), frameIndex_, batch_, layout);
    }

    void FillObjectConstants(float seconds, float scale, rs2::ObjectConstants* out) const
    {
        D3DXMATRIX scaling, spinY, spinX, world, view, projection;
        D3DXMatrixScaling(&scaling, scale, scale, scale);
        D3DXMatrixRotationY(&spinY, seconds);
        D3DXMatrixRotationX(&spinX, seconds * 0.5f);
        world = scaling * spinY * spinX;

        const D3DXVECTOR3 eye(0.0f, 1.5f, -4.0f), at(0.0f, 0.0f, 0.0f), up(0.0f, 1.0f, 0.0f);
        D3DXMatrixLookAtLH(&view, &eye, &at, &up);
        const float aspect = static_cast<float>(width_) / static_cast<float>(height_);
        D3DXMatrixPerspectiveFovLH(&projection, D3DXToRadian(45.0f), aspect, 0.1f, 100.0f);

        const D3DXMATRIX mvp = world * view * projection;
        std::memcpy(out->mvp, mvp.m, sizeof(out->mvp));
        std::memcpy(out->world, world.m, sizeof(out->world));
        out->material = state_.GetMaterial();
    }

    bool DrawMesh(const Mesh& mesh, const rs2::ObjectConstants& object)
    {
        std::string error;
        ID3D12PipelineState* pso = pipelines_.Get(state_.Pipeline(), &error);
        if (pso == nullptr) {
            MessageBoxA(window_, error.c_str(), "RailSim2 renderer error", MB_OK | MB_ICONERROR);
            return false;
        }
        const D3D12_GPU_VIRTUAL_ADDRESS frameConstants = constants_.Upload(state_.Constants());
        if (frameConstants == 0) {
            MessageBoxA(window_, "Constant ring exhausted", "RailSim2 renderer error", MB_OK | MB_ICONERROR);
            return false;
        }
        commandList_->SetPipelineState(pso);
        commandList_->SetGraphicsRoot32BitConstants(0, sizeof(object) / 4, &object, 0);
        commandList_->SetGraphicsRootConstantBufferView(1, frameConstants);
        commandList_->IASetVertexBuffers(0, 1, &mesh.vertexView);
        commandList_->IASetIndexBuffer(&mesh.indexView);
        commandList_->DrawIndexedInstanced(mesh.indexCount, 1, 0, 0, 0);
        return true;
    }

    bool DrawScene()
    {
        const D3D12_VIEWPORT viewport{0.0f, 0.0f, static_cast<float>(width_), static_cast<float>(height_), 0.0f, 1.0f};
        const D3D12_RECT scissor{0, 0, static_cast<LONG>(width_), static_cast<LONG>(height_)};
        commandList_->SetGraphicsRootSignature(pipelines_.RootSignature());
        ID3D12DescriptorHeap* heaps[] = {srvHeap_.Get()};
        commandList_->SetDescriptorHeaps(1, heaps);
        commandList_->SetGraphicsRootDescriptorTable(2, srvHeap_->GetGPUDescriptorHandleForHeapStart());
        commandList_->RSSetViewports(1, &viewport);
        commandList_->RSSetScissorRects(1, &scissor);
        commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        constants_.BeginFrame(frameIndex_);
        const float seconds = static_cast<float>(GetTickCount64() - startTime_) / 1000.0f;
        rs2::ObjectConstants object{};

        // Opaque, lit cube with fog: warm key light plus a cool fill light.
        state_ = rs2::RenderState();
        state_.SetLighting(true);
        state_.SetTextured(true);
        state_.SetAmbient(0xFF303030u);
        state_.SetDirectionalLight(0, -0.4f, -1.0f, 0.6f, 0xFFFFF0E0u);
        state_.SetDirectionalLight(1, 0.8f, 0.2f, -0.4f, 0xFF405070u);
        state_.SetLight(0, true);
        state_.SetLight(1, true);
        state_.SetFog(true, 0xFF152033u, 3.0f, 9.0f);
        FillObjectConstants(seconds, 1.0f, &object);
        if (!DrawMesh(opaqueCube_, object)) {
            return false;
        }

        // Translucent shell: alpha blend, no depth write, no culling, untextured, emissive tint.
        state_.SetTextured(false);
        state_.SetZWrite(false);
        state_.SetCulling(false);
        state_.BlendAlpha();
        rs2::Material glassMaterial;
        glassMaterial.emissive[2] = 0.15f;
        state_.SetMaterial(glassMaterial);
        FillObjectConstants(seconds, 1.6f, &object);
        return DrawMesh(glassCube_, object);
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
    ComPtr<ID3D12DescriptorHeap> dsvHeap_;
    ComPtr<ID3D12DescriptorHeap> srvHeap_;
    ComPtr<ID3D12Resource> texture_;
    rs2::UiRenderer ui_;
    rs2::SpriteBatch batch_;
    rs2::GlyphTable glyphs_;
    ComPtr<ID3D12Resource> whiteTexture_;
    ComPtr<ID3D12Resource> fontTexture_;
    int fontPixelHeight_ = 0;
    ULONGLONG fpsWindowStart_ = 0;
    UINT framesSinceFpsUpdate_ = 0;
    float fps_ = 0.0f;
    ComPtr<ID3D12Resource> depthBuffer_;
    rs2::PipelineCache pipelines_;
    rs2::ConstantRing constants_;
    rs2::RenderState state_;
    Mesh opaqueCube_;
    Mesh glassCube_;
    ULONGLONG startTime_ = 0;
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

    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
        ShowWin32Error(nullptr, L"CoInitializeEx", GetLastError());
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
        if (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                break;
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
            continue;
        }
        // Idle: render continuously (vsync paces the loop through Present).
        if (!renderer.Render()) {
            g_renderer = nullptr;
            return 1;
        }
    }

    g_renderer = nullptr;
    return static_cast<int>(message.wParam);
}
