#pragma once

#include <d3d11.h>
#include "imgui.h"

namespace blur
{
    bool initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void set_source(ID3D11ShaderResourceView* source);
    bool update();
    void shutdown();
}

void draw_blur(ImDrawList* draw_list);
void draw_blur_region(ImDrawList* draw_list, const ImVec2& min, const ImVec2& max, float rounding);
