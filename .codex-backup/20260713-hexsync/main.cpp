// Dear ImGui: Win32 + DirectX 11 standalone menu
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <d3d11.h>
#include <d3dx11.h>
#include <tchar.h>

#include "gui.hpp"
#include "hashes.hpp"
#include "blur.hpp"
#include "bytes.hpp"
#include "neverlose_menu.hpp"

using namespace ImGui;

#define ALPHA    ( ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoBorder )
#define NO_ALPHA ( ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoBorder )

static constexpr float kDpiScale = 1.0f;

ID3D11ShaderResourceView* avatar{};
ID3D11ShaderResourceView* bg{};
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void ScaleDrawData(ImDrawData* draw_data, float scale);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

const char* FirstExistingFont(const char* const* candidates, int count)
{
    for (int index = 0; index < count; ++index)
    {
        if (::GetFileAttributesA(candidates[index]) != INVALID_FILE_ATTRIBUTES)
            return candidates[index];
    }
    return nullptr;
}

int main(int, char**)
{
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Neverlose DX11", nullptr };
    ::RegisterClassExW(&wc);
    constexpr DWORD window_style = WS_POPUP;
    const int window_x = (::GetSystemMetrics(SM_CXSCREEN) - 750) / 2;
    const int window_y = (::GetSystemMetrics(SM_CYSCREEN) - 756) / 2;
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Neverlose ImGui - DirectX 11", window_style,
        window_x, window_y, 750, 756,
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplWin32_SetLogicalScale(kDpiScale);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    blur::initialize(g_pd3dDevice, g_pd3dDeviceContext);

    // Rasterize at the physical 125% size, then keep the logical layout at
    // 100%. After draw vertices are scaled, glyphs land at their native atlas
    // resolution instead of being magnified and blurred.
    io.FontGlobalScale = 1.0f / kDpiScale;
    const char* regular_candidates[] = {
        "PingFangSC-Regular.ttf",
        "PingFang SC Regular.ttf",
        "PingFang.ttc",
        "..\\PingFangSC-Regular.ttf",
        "..\\PingFang SC Regular.ttf",
        "..\\PingFang.ttc",
        "C:\\Windows\\Fonts\\PingFangSC-Regular.ttf",
        "C:\\Windows\\Fonts\\PingFang.ttc"
    };
    const char* regular_font = FirstExistingFont(regular_candidates, IM_ARRAYSIZE(regular_candidates));
    if (!regular_font)
        regular_font = "C:\\Windows\\Fonts\\seguisb.ttf";

    ImFontConfig text_config;
    text_config.OversampleH = 3;
    text_config.PixelSnapH = true;
    text_config.RasterizerMultiply = 1.0f;
    const ImWchar* text_ranges = io.Fonts->GetGlyphRangesChineseSimplifiedCommon();
    io.Fonts->AddFontFromFileTTF("..\\SSTMedium.TTF", 15.0f * kDpiScale, &text_config);

    ImFontConfig pingfang_config;
    pingfang_config.MergeMode = true;
    pingfang_config.OversampleH = 3;
    pingfang_config.PixelSnapH = true;
    pingfang_config.RasterizerMultiply = 1.15f;
    io.Fonts->AddFontFromFileTTF(regular_font, 15.0f * kDpiScale, &pingfang_config, text_ranges);
    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF("..\\fa-solid-900.ttf", 14.0f * kDpiScale, &icons_config, icon_ranges);
    io.Fonts->AddFontFromFileTTF("..\\SSTBold.TTF", 16.0f * kDpiScale, &text_config);
    io.Fonts->AddFontFromFileTTF(regular_font, 16.0f * kDpiScale, &pingfang_config, text_ranges);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (!avatar)
            D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, &esliboganet, sizeof esliboganet, nullptr, nullptr, &avatar, nullptr);

        if (!bg)
        {
            D3DX11CreateShaderResourceViewFromFileA(g_pd3dDevice, "C:\\Windows\\Web\\Screen\\img104.jpg", nullptr, nullptr, &bg, nullptr);
            blur::set_source(bg);
            blur::update();
        }
        RenderNeverloseMenu(bg, avatar);
        ImGui::EndFrame();
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        ScaleDrawData(draw_data, kDpiScale);

        const float clear_color_with_alpha[4] = {
            clear_color.x * clear_color.w,
            clear_color.y * clear_color.w,
            clear_color.z * clear_color.w,
            clear_color.w
        };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(draw_data);
        g_pSwapChain->Present(1, 0);
    }

    blur::shutdown();
    if (avatar) { avatar->Release(); avatar = nullptr; }
    if (bg) { bg->Release(); bg = nullptr; }
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

void ScaleDrawData(ImDrawData* draw_data, float scale)
{
    if (!draw_data || scale == 1.0f)
        return;

    for (int list_index = 0; list_index < draw_data->CmdListsCount; ++list_index)
    {
        ImDrawList* list = draw_data->CmdLists[list_index];
        for (int vertex_index = 0; vertex_index < list->VtxBuffer.Size; ++vertex_index)
        {
            list->VtxBuffer[vertex_index].pos.x *= scale;
            list->VtxBuffer[vertex_index].pos.y *= scale;
        }
    }

    draw_data->ScaleClipRects(ImVec2(scale, scale));
    draw_data->DisplaySize.x *= scale;
    draw_data->DisplaySize.y *= scale;
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT create_device_flags = 0;
#ifdef _DEBUG
    create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL feature_level;
    const D3D_FEATURE_LEVEL feature_level_array[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_device_flags,
        feature_level_array, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &feature_level, &g_pd3dDeviceContext);
    if (result == DXGI_ERROR_UNSUPPORTED)
        result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, create_device_flags,
            feature_level_array, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &feature_level, &g_pd3dDeviceContext);
    if (result != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* back_buffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    if (back_buffer)
    {
        g_pd3dDevice->CreateRenderTargetView(back_buffer, nullptr, &g_mainRenderTargetView);
        back_buffer->Release();
    }
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
