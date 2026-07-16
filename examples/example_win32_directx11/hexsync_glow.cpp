#include "hexsync_intro.hpp"

#include <array>
#include <cmath>
#include <cstdint>

ID3D11ShaderResourceView* CreateHexSyncGlowTexture(ID3D11Device* device)
{
    if (!device)
        return nullptr;

    constexpr int texture_size = 128;
    std::array<std::uint32_t, texture_size * texture_size> pixels{};
    for (int y = 0; y < texture_size; ++y)
    {
        for (int x = 0; x < texture_size; ++x)
        {
            const float nx = (x + 0.5f) / (texture_size * 0.5f) - 1.0f;
            const float ny = (y + 0.5f) / (texture_size * 0.5f) - 1.0f;
            const float distance = std::sqrt(nx * nx + ny * ny);
            const float alpha = std::pow(distance < 1.0f ? 1.0f - distance : 0.0f, 1.7f);
            pixels[y * texture_size + x] = 0x00FFFFFFu | (static_cast<std::uint32_t>(alpha * 255.0f) << 24);
        }
    }

    D3D11_TEXTURE2D_DESC description{};
    description.Width = texture_size;
    description.Height = texture_size;
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_IMMUTABLE;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = pixels.data();
    data.SysMemPitch = texture_size * sizeof(std::uint32_t);

    ID3D11Texture2D* texture = nullptr;
    if (FAILED(device->CreateTexture2D(&description, &data, &texture)))
        return nullptr;
    ID3D11ShaderResourceView* result = nullptr;
    device->CreateShaderResourceView(texture, nullptr, &result);
    texture->Release();
    return result;
}
