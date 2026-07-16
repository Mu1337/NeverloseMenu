#pragma once

#include <d3d11.h>

struct ImFont;

ID3D11ShaderResourceView* CreateHexSyncGlowTexture(ID3D11Device* device);
bool RenderHexSyncIntro(ID3D11ShaderResourceView* logo, ID3D11ShaderResourceView* glow, ImFont* wordmark_font);
