#include "blur.hpp"

#include <d3dcompiler.h>
#include <cstring>

#pragma comment(lib, "d3dcompiler.lib")

namespace
{
    ID3D11Device* g_device = nullptr;
    ID3D11DeviceContext* g_context = nullptr;
    ID3D11ShaderResourceView* g_source = nullptr;
    ID3D11VertexShader* g_vertex_shader = nullptr;
    ID3D11PixelShader* g_pixel_shader = nullptr;
    ID3D11SamplerState* g_sampler = nullptr;
    ID3D11Buffer* g_constants = nullptr;
    ID3D11Texture2D* g_horizontal_texture = nullptr;
    ID3D11RenderTargetView* g_horizontal_rtv = nullptr;
    ID3D11ShaderResourceView* g_horizontal_srv = nullptr;
    ID3D11Texture2D* g_blurred_texture = nullptr;
    ID3D11RenderTargetView* g_blurred_rtv = nullptr;
    ID3D11ShaderResourceView* g_blurred_srv = nullptr;
    UINT g_width = 0;
    UINT g_height = 0;
    bool g_dirty = true;

    template <typename T>
    void release(T*& object)
    {
        if (object)
        {
            object->Release();
            object = nullptr;
        }
    }

    void release_targets()
    {
        release(g_horizontal_srv);
        release(g_horizontal_rtv);
        release(g_horizontal_texture);
        release(g_blurred_srv);
        release(g_blurred_rtv);
        release(g_blurred_texture);
        g_width = g_height = 0;
    }

    bool compile_shader(const char* source, const char* entry, const char* profile, ID3DBlob** blob)
    {
        ID3DBlob* errors = nullptr;
        const HRESULT result = D3DCompile(source, std::strlen(source), "imgui_gaussian_blur", nullptr, nullptr,
            entry, profile, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, blob, &errors);
        release(errors);
        return SUCCEEDED(result);
    }

    bool create_targets(UINT width, UINT height)
    {
        if (width == g_width && height == g_height && g_blurred_srv)
            return true;

        release_targets();
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        if (FAILED(g_device->CreateTexture2D(&desc, nullptr, &g_horizontal_texture)) ||
            FAILED(g_device->CreateRenderTargetView(g_horizontal_texture, nullptr, &g_horizontal_rtv)) ||
            FAILED(g_device->CreateShaderResourceView(g_horizontal_texture, nullptr, &g_horizontal_srv)) ||
            FAILED(g_device->CreateTexture2D(&desc, nullptr, &g_blurred_texture)) ||
            FAILED(g_device->CreateRenderTargetView(g_blurred_texture, nullptr, &g_blurred_rtv)) ||
            FAILED(g_device->CreateShaderResourceView(g_blurred_texture, nullptr, &g_blurred_srv)))
        {
            release_targets();
            return false;
        }

        g_width = width;
        g_height = height;
        return true;
    }

    void render_pass(ID3D11ShaderResourceView* input, ID3D11RenderTargetView* output, float x, float y)
    {
        const float constants[4] = { x, y, 0.0f, 0.0f };
        g_context->UpdateSubresource(g_constants, 0, nullptr, constants, 0, 0);
        g_context->OMSetRenderTargets(1, &output, nullptr);
        g_context->PSSetShaderResources(0, 1, &input);
        g_context->Draw(3, 0);

        ID3D11ShaderResourceView* null_srv = nullptr;
        g_context->PSSetShaderResources(0, 1, &null_srv);
    }
}

bool blur::initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
    if (g_device == device && g_context == context && g_pixel_shader)
        return true;
    shutdown();
    if (!device || !context)
        return false;

    g_device = device;
    g_context = context;
    g_device->AddRef();
    g_context->AddRef();

    static const char* shader_source = R"(
        struct VSOutput { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };
        VSOutput VSMain(uint id : SV_VertexID)
        {
            VSOutput output;
            output.uv = float2((id << 1) & 2, id & 2);
            output.position = float4(output.uv.x * 2.0 - 1.0, 1.0 - output.uv.y * 2.0, 0.0, 1.0);
            return output;
        }

        Texture2D source_texture : register(t0);
        SamplerState linear_sampler : register(s0);
        cbuffer BlurConstants : register(b0) { float2 direction; float2 padding; };
        float4 PSMain(VSOutput input) : SV_TARGET
        {
            float4 color = source_texture.Sample(linear_sampler, input.uv) * 0.2270270270;
            color += source_texture.Sample(linear_sampler, input.uv + direction * 1.3846153846) * 0.3162162162;
            color += source_texture.Sample(linear_sampler, input.uv - direction * 1.3846153846) * 0.3162162162;
            color += source_texture.Sample(linear_sampler, input.uv + direction * 3.2307692308) * 0.0702702703;
            color += source_texture.Sample(linear_sampler, input.uv - direction * 3.2307692308) * 0.0702702703;
            return color;
        }
    )";

    ID3DBlob* vs_blob = nullptr;
    ID3DBlob* ps_blob = nullptr;
    if (!compile_shader(shader_source, "VSMain", "vs_4_0", &vs_blob) ||
        !compile_shader(shader_source, "PSMain", "ps_4_0", &ps_blob) ||
        FAILED(g_device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &g_vertex_shader)) ||
        FAILED(g_device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &g_pixel_shader)))
    {
        release(vs_blob);
        release(ps_blob);
        shutdown();
        return false;
    }
    release(vs_blob);
    release(ps_blob);

    D3D11_SAMPLER_DESC sampler_desc{};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

    D3D11_BUFFER_DESC buffer_desc{};
    buffer_desc.ByteWidth = 16;
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(g_device->CreateSamplerState(&sampler_desc, &g_sampler)) ||
        FAILED(g_device->CreateBuffer(&buffer_desc, nullptr, &g_constants)))
    {
        shutdown();
        return false;
    }
    return true;
}

void blur::set_source(ID3D11ShaderResourceView* source)
{
    if (g_source == source)
        return;
    release(g_source);
    g_source = source;
    if (g_source)
        g_source->AddRef();
    g_dirty = true;
}

bool blur::update()
{
    if (!g_dirty || !g_source)
        return g_blurred_srv != nullptr;

    ID3D11Resource* resource = nullptr;
    g_source->GetResource(&resource);
    ID3D11Texture2D* texture = nullptr;
    if (!resource || FAILED(resource->QueryInterface(IID_PPV_ARGS(&texture))))
    {
        release(resource);
        return false;
    }
    D3D11_TEXTURE2D_DESC source_desc{};
    texture->GetDesc(&source_desc);
    release(texture);
    release(resource);
    if (!create_targets(source_desc.Width, source_desc.Height))
        return false;

    D3D11_VIEWPORT old_viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
    UINT old_viewport_count = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    g_context->RSGetViewports(&old_viewport_count, old_viewports);
    ID3D11RenderTargetView* old_rtv = nullptr;
    ID3D11DepthStencilView* old_dsv = nullptr;
    g_context->OMGetRenderTargets(1, &old_rtv, &old_dsv);

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(g_width);
    viewport.Height = static_cast<float>(g_height);
    viewport.MaxDepth = 1.0f;
    g_context->RSSetViewports(1, &viewport);
    g_context->IASetInputLayout(nullptr);
    g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_context->VSSetShader(g_vertex_shader, nullptr, 0);
    g_context->PSSetShader(g_pixel_shader, nullptr, 0);
    g_context->PSSetSamplers(0, 1, &g_sampler);
    g_context->PSSetConstantBuffers(0, 1, &g_constants);

    render_pass(g_source, g_horizontal_rtv, 1.0f / static_cast<float>(g_width), 0.0f);
    render_pass(g_horizontal_srv, g_blurred_rtv, 0.0f, 1.0f / static_cast<float>(g_height));

    g_context->OMSetRenderTargets(1, &old_rtv, old_dsv);
    if (old_viewport_count > 0)
        g_context->RSSetViewports(old_viewport_count, old_viewports);
    release(old_rtv);
    release(old_dsv);
    g_dirty = false;
    return true;
}

void blur::shutdown()
{
    release_targets();
    release(g_source);
    release(g_constants);
    release(g_sampler);
    release(g_pixel_shader);
    release(g_vertex_shader);
    release(g_context);
    release(g_device);
    g_dirty = true;
}

void draw_blur(ImDrawList* draw_list)
{
    if (!draw_list || !g_blurred_srv)
        return;
    draw_list->AddImage(g_blurred_srv, ImVec2(0.0f, 0.0f), ImGui::GetIO().DisplaySize);
}

void draw_blur_region(ImDrawList* draw_list, const ImVec2& min, const ImVec2& max, float rounding)
{
    if (!draw_list || !g_blurred_srv)
        return;

    const ImVec2 display_size = ImGui::GetIO().DisplaySize;
    if (display_size.x <= 0.0f || display_size.y <= 0.0f)
        return;

    const ImVec2 uv_min(min.x / display_size.x, min.y / display_size.y);
    const ImVec2 uv_max(max.x / display_size.x, max.y / display_size.y);
    draw_list->AddImageRounded(g_blurred_srv, min, max, uv_min, uv_max,
        IM_COL32_WHITE, rounding, ImDrawFlags_RoundCornersAll);
}
