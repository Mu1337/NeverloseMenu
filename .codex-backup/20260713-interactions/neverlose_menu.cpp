#include "neverlose_menu.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

#include "blur.hpp"
#include "hashes.hpp"

#include <cstdio>
#include <cmath>
#include <string>

namespace
{
    constexpr ImVec2 kCanvasSize(750.0f, 756.0f);
    constexpr ImVec2 kShellSize(748.0f, 576.0f);
    constexpr float kPopoverRowsY = 82.0f;
    constexpr float kPopoverRowHeight = 32.0f;
    constexpr float kTextBody = 15.0f;
    constexpr float kTextControl = 14.0f;
    constexpr float kTextSmall = 12.0f;
    constexpr float kTextCaption = 10.0f;
    constexpr float kTextTitle = 16.0f;
    constexpr float kTextIcon = 13.0f;

    struct MenuState
    {
        int navigation = 0;
        int dropdown_id = -1;
        int dropdown_opened_frame = -1;
        int refine_shot = 1;
        int history = 0;
        int delay_shot = 0;
        int remove_spread = 0;
        int hitbox = 0;
        int multipoint = 0;
        int leg_movement = 0;
        bool dropdown_open = false;
        bool account_open = true;
        bool enabled = true;
        bool silent_aim = true;
        bool automatic_fire = true;
        bool aim_through_walls = true;
        bool duck_peek_assist = false;
        bool quick_peek_assist = false;
        bool double_tap = false;
        bool safe_points = true;
        bool body_aim = true;
        bool anti_aim = true;
        bool suppress_breathing = true;
        bool synchronization = false;
        float field_of_view = 180.0f;
        float hitchance = 0.0f;
        float minimum_damage = 20.0f;
    };

    MenuState state;
    struct DropdownContext
    {
        bool valid = false;
        int id = -1;
        ImVec2 control;
        float width = 0.0f;
        int* selection = nullptr;
        const char* const* options = nullptr;
        int option_count = 0;
    };
    DropdownContext dropdown;
    const char* const kRefineOptions[] = { "Off", "Latency", "Performance" };
    const char* const kHistoryOptions[] = { "Maximum", "Last Record", "All Records" };
    const char* const kDelayOptions[] = { "Select", "Damage", "Hit Chance" };
    const char* const kSpreadOptions[] = { "Full", "Compensated", "Disabled" };
    const char* const kHitboxOptions[] = { "Head", "Chest", "Stomach" };
    const char* const kMultipointOptions[] = { "Head, Stomach", "Head", "Full" };
    const char* const kLegOptions[] = { "Sliding", "Walking", "Static" };

    ImU32 Color(int red, int green, int blue, int alpha = 255)
    {
        return IM_COL32(red, green, blue, alpha);
    }

    float Motion(ImGuiID id, float target, float speed = 14.0f, float initial = -1.0f)
    {
        float* value = ImGui::GetStateStorage()->GetFloatRef(id, initial < 0.0f ? target : initial);
        const float blend = 1.0f - std::exp(-speed * ImGui::GetIO().DeltaTime);
        *value = ImLerp(*value, target, blend);
        if (std::fabs(*value - target) < 0.0005f)
            *value = target;
        return *value;
    }

    ImU32 MixColor(ImU32 from, ImU32 to, float amount)
    {
        return ImGui::ColorConvertFloat4ToU32(ImLerp(
            ImGui::ColorConvertU32ToFloat4(from), ImGui::ColorConvertU32ToFloat4(to), ImSaturate(amount)));
    }

    void Text(ImDrawList* draw, const ImVec2& position, ImU32 color, const char* value, float size = kTextBody, ImFont* font = nullptr)
    {
        draw->AddText(font ? font : ImGui::GetFont(), size, position, color, value);
    }

    void TextCenteredY(ImDrawList* draw, float x, float top, float height, ImU32 color, const char* value, float size, ImFont* font = nullptr)
    {
        ImFont* resolved_font = font ? font : ImGui::GetFont();
        const float text_height = resolved_font->CalcTextSizeA(size, FLT_MAX, 0.0f, value).y;
        Text(draw, ImVec2(x, top + ImFloor((height - text_height) * 0.5f)), color, value, size, resolved_font);
    }

    bool HitTarget(const char* id, const ImVec2& position, const ImVec2& size)
    {
        ImGui::SetCursorScreenPos(position);
        return ImGui::InvisibleButton(id, size);
    }

    void Chevron(ImDrawList* draw, const ImVec2& position, ImU32 color)
    {
        draw->AddLine(position, position + ImVec2(3.0f, 3.0f), color, 1.3f);
        draw->AddLine(position + ImVec2(3.0f, 3.0f), position + ImVec2(0.0f, 6.0f), color, 1.3f);
    }

    void MoreDots(ImDrawList* draw, const ImVec2& position)
    {
        for (int index = 0; index < 3; ++index)
            draw->AddCircleFilled(position + ImVec2(index * 3.0f, 0.0f), 1.0f, Color(180, 184, 195));
    }

    bool Toggle(ImDrawList* draw, const char* id, const ImVec2& position, bool& value)
    {
        if (HitTarget(id, position - ImVec2(5.0f, 6.0f), ImVec2(39.0f, 30.0f)))
            value = !value;

        const ImGuiID item_id = ImGui::GetItemID();
        const float amount = Motion(item_id ^ 0x54A44E11u, value ? 1.0f : 0.0f, 18.0f);
        const float hover = Motion(item_id ^ 0x0B87D6A9u, ImGui::IsItemHovered() ? 1.0f : 0.0f, 20.0f);
        const float press = Motion(item_id ^ 0x7FB8C124u, ImGui::IsItemActive() ? 1.0f : 0.0f, 26.0f);
        const ImU32 track = MixColor(Color(31, 35, 45), Color(75, 126, 255), amount);
        const ImU32 thumb = MixColor(Color(132, 144, 156), Color(246, 248, 255), amount);
        draw->AddRectFilled(position - ImVec2(1.0f + hover, 1.0f + hover),
            position + ImVec2(30.0f + hover, 19.0f + hover), Color(75, 126, 255, static_cast<int>(55.0f * hover)), 10.0f);
        draw->AddRectFilled(position, position + ImVec2(29.0f, 18.0f), track, 9.0f);
        draw->AddCircleFilled(position + ImVec2(ImLerp(9.0f, 20.0f, amount), 9.0f), 7.0f - press * 0.8f, thumb);
        return value;
    }

    void Card(ImDrawList* draw, const ImVec2& position, const ImVec2& size, const char* title)
    {
        Text(draw, position + ImVec2(12.0f, -18.0f), Color(91, 96, 108), title, kTextCaption);
        draw->AddRectFilled(position, position + size, Color(17, 19, 27), 13.0f);
        draw->AddRect(position, position + size, Color(31, 34, 44), 13.0f, 0, 1.0f);
    }

    void RowBase(ImDrawList* draw, const ImVec2& card, float width, int row, const char* label)
    {
        const float y = card.y + row * 37.0f;
        TextCenteredY(draw, card.x + 13.0f, y, 37.0f, Color(207, 209, 218), label, kTextBody);
        if (row > 0)
            draw->AddLine(ImVec2(card.x + 12.0f, y), ImVec2(card.x + width - 12.0f, y), Color(28, 31, 40), 1.0f);
    }

    void ToggleRow(ImDrawList* draw, const ImVec2& card, float width, int row, const char* id, const char* label, bool& value, bool dots = false)
    {
        RowBase(draw, card, width, row, label);
        const float y = card.y + row * 37.0f;
        if (dots)
            MoreDots(draw, ImVec2(card.x + width - 66.0f, y + 18.0f));
        Toggle(draw, id, ImVec2(card.x + width - 43.0f, y + 9.0f), value);
    }

    void SelectRow(ImDrawList* draw, const ImVec2& card, float width, int row, int select_id,
        const char* label, int& selection, const char* const* options, int option_count, bool dots = false)
    {
        RowBase(draw, card, width, row, label);
        const float y = card.y + row * 37.0f;
        const float control_width = 126.0f;
        const ImVec2 control(card.x + width - control_width - 13.0f, y + 7.0f);
        ImGui::PushID(select_id);
        const bool clicked = HitTarget("##select", control, ImVec2(control_width, 23.0f));
        const ImGuiID item_id = ImGui::GetItemID();
        const bool hovered = ImGui::IsItemHovered();
        const bool active = ImGui::IsItemActive();
        ImGui::PopID();
        if (clicked)
        {
            state.dropdown_open = state.dropdown_id != select_id || !state.dropdown_open;
            state.dropdown_id = select_id;
            if (state.dropdown_open)
                state.dropdown_opened_frame = ImGui::GetFrameCount();
        }
        const bool open = state.dropdown_id == select_id && state.dropdown_open;
        const float response = Motion(item_id ^ 0x1E5A67C3u, open ? 1.0f : active ? 0.9f : hovered ? 0.65f : 0.0f, 18.0f);
        if (state.dropdown_id == select_id)
            dropdown = { true, select_id, control, control_width, &selection, options, option_count };
        if (dots)
            MoreDots(draw, ImVec2(control.x - 17.0f, y + 18.0f));
        draw->AddRectFilled(control, control + ImVec2(control_width, 23.0f),
            MixColor(Color(25, 28, 38), Color(31, 38, 54), response), 5.0f);
        draw->AddRect(control, control + ImVec2(control_width, 23.0f),
            MixColor(Color(31, 35, 46), Color(75, 126, 255), response), 5.0f);
        TextCenteredY(draw, control.x + 7.0f, control.y, 23.0f, Color(169, 172, 183), options[selection], kTextControl);
        const float arrow_y = response * 1.5f;
        draw->AddLine(control + ImVec2(control_width - 13.0f, 9.0f + arrow_y), control + ImVec2(control_width - 9.0f, 13.0f + arrow_y), Color(137, 141, 151), 1.0f);
        draw->AddLine(control + ImVec2(control_width - 9.0f, 13.0f + arrow_y), control + ImVec2(control_width - 5.0f, 9.0f + arrow_y), Color(137, 141, 151), 1.0f);
    }

    void ChevronRow(ImDrawList* draw, const ImVec2& card, float width, int row, const char* label)
    {
        RowBase(draw, card, width, row, label);
        const ImVec2 row_position(card.x, card.y + row * 37.0f);
        HitTarget(label, row_position, ImVec2(width, 37.0f));
        const float hover = Motion(ImGui::GetItemID() ^ 0x5239ACD1u, ImGui::IsItemHovered() ? 1.0f : 0.0f, 18.0f);
        if (hover > 0.001f)
            draw->AddRectFilled(row_position + ImVec2(4.0f, 3.0f), row_position + ImVec2(width - 4.0f, 34.0f),
                Color(75, 126, 255, static_cast<int>(18.0f * hover)), 5.0f);
        Chevron(draw, ImVec2(card.x + width - 24.0f + hover * 2.0f, row_position.y + 15.0f), Color(186, 189, 198));
    }

    void SliderRow(ImDrawList* draw, const ImVec2& card, float width, int row, const char* id,
        const char* label, float& value, float maximum, const char* format)
    {
        RowBase(draw, card, width, row, label);
        const float y = card.y + row * 37.0f;
        const ImVec2 start(card.x + width - 140.0f, y + 18.0f);
        const float track_width = 72.0f;
        HitTarget(id, start - ImVec2(5.0f, 8.0f), ImVec2(track_width + 10.0f, 19.0f));
        if (ImGui::IsItemActive())
            value = ImClamp((ImGui::GetIO().MousePos.x - start.x) / track_width, 0.0f, 1.0f) * maximum;
        const ImGuiID item_id = ImGui::GetItemID();
        const float shown = Motion(item_id ^ 0x19D5B86Fu, value / maximum, 12.0f, 0.0f);
        const float hover = Motion(item_id ^ 0x65F01AC3u,
            ImGui::IsItemActive() ? 1.0f : ImGui::IsItemHovered() ? 0.7f : 0.0f, 18.0f);
        draw->AddRectFilled(start, start + ImVec2(track_width, 3.0f), MixColor(Color(34, 38, 48), Color(43, 50, 65), hover), 1.5f);
        const float fill_width = track_width * shown;
        if (fill_width > 0.1f)
            draw->AddRectFilledMultiColor(start, start + ImVec2(fill_width, 3.0f),
                Color(69, 104, 255), Color(84, 151, 255), Color(84, 151, 255), Color(69, 104, 255));
        const ImVec2 thumb = start + ImVec2(fill_width, 1.5f);
        draw->AddCircleFilled(thumb, 8.0f + hover * 2.0f, Color(75, 126, 255, static_cast<int>(30.0f + 40.0f * hover)), 24);
        draw->AddCircleFilled(thumb, 5.0f + hover * 0.6f, Color(247, 248, 252), 20);
        const ImVec2 pill(card.x + width - 56.0f, y + 8.0f);
        draw->AddRectFilled(pill, pill + ImVec2(43.0f, 21.0f), Color(25, 28, 38), 5.0f);
        char value_text[24];
        std::snprintf(value_text, sizeof(value_text), format, shown * maximum);
        TextCenteredY(draw, pill.x + 5.0f, pill.y, 21.0f, Color(166, 169, 179), value_text, kTextControl);
    }

    void DropdownPopup(ImDrawList* draw)
    {
        if (!dropdown.valid)
        {
            state.dropdown_open = false;
            return;
        }
        const ImVec2 panel(dropdown.control.x, dropdown.control.y + 27.0f);
        const ImVec2 panel_size(dropdown.width, dropdown.option_count * 32.0f + 8.0f);
        const bool over_panel = ImGui::IsMouseHoveringRect(panel, panel + panel_size);
        const bool over_control = ImGui::IsMouseHoveringRect(dropdown.control,
            dropdown.control + ImVec2(dropdown.width, 23.0f));
        if (ImGui::GetFrameCount() != state.dropdown_opened_frame &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) && (!over_panel || over_control))
            state.dropdown_open = false;

        ImGui::PushID(dropdown.id);
        const float open = Motion(ImGui::GetID("##dropdown_motion"), state.dropdown_open ? 1.0f : 0.0f, 18.0f, 0.0f);
        ImGui::PopID();
        if (open <= 0.001f)
            return;

        const int first_vertex = draw->VtxBuffer.Size;
        constexpr float rounding = 15.0f;
        draw->AddRectFilled(panel - ImVec2(3.0f, 1.0f), panel + panel_size + ImVec2(3.0f, 9.0f), Color(0, 0, 0, 30), rounding + 3.0f);
        draw->AddRectFilled(panel - ImVec2(1.0f, 0.0f), panel + panel_size + ImVec2(1.0f, 5.0f), Color(0, 0, 0, 70), rounding + 1.0f);
        draw_blur_region(draw, panel, panel + panel_size, rounding);
        draw->AddRectFilled(panel, panel + panel_size, Color(18, 19, 28, 214), rounding);
        draw->AddRect(panel, panel + panel_size, Color(57, 61, 75, 190), rounding);
        draw->AddLine(panel + ImVec2(15.0f, 1.0f), panel + ImVec2(panel_size.x - 15.0f, 1.0f), Color(255, 255, 255, 20), 1.0f);
        for (int index = 0; index < dropdown.option_count; ++index)
        {
            const ImVec2 row = panel + ImVec2(4.0f, 4.0f + index * 32.0f);
            ImGui::PushID(dropdown.id * 16 + index);
            const bool clicked = HitTarget("##option", row, ImVec2(dropdown.width - 8.0f, 32.0f));
            const ImGuiID item_id = ImGui::GetItemID();
            const bool hovered = ImGui::IsItemHovered();
            const float hover = Motion(item_id ^ 0x68A7C251u, hovered ? 1.0f : 0.0f, 20.0f);
            ImGui::PopID();
            if (clicked || (hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left)))
            {
                *dropdown.selection = index;
                state.dropdown_open = false;
            }
            if (hover > 0.001f || *dropdown.selection == index)
                draw->AddRectFilled(row, row + ImVec2(dropdown.width - 8.0f, 32.0f),
                    Color(75, 126, 255, static_cast<int>((*dropdown.selection == index ? 34.0f : 24.0f) * ImMax(hover, 0.7f))), 10.0f);
            TextCenteredY(draw, row.x + 14.0f, row.y, 32.0f,
                *dropdown.selection == index ? Color(217, 225, 247) : Color(182, 185, 196),
                dropdown.options[index], kTextControl);
        }
        const float eased = 1.0f - std::pow(1.0f - open, 3.0f);
        const ImVec2 pivot(panel.x + panel_size.x * 0.5f, panel.y);
        for (int index = first_vertex; index < draw->VtxBuffer.Size; ++index)
        {
            ImDrawVert& vertex = draw->VtxBuffer[index];
            vertex.pos = pivot + (vertex.pos - pivot) * ImLerp(0.94f, 1.0f, eased) + ImVec2(0.0f, (1.0f - eased) * 8.0f);
            const ImU32 alpha = (vertex.col >> IM_COL32_A_SHIFT) & 0xFFu;
            vertex.col = (vertex.col & 0x00FFFFFFu) |
                (static_cast<ImU32>(alpha * eased) << IM_COL32_A_SHIFT);
        }
    }

    void Sidebar(ImDrawList* draw, const ImVec2& base, ID3D11ShaderResourceView* avatar)
    {
        ImFont* strong = ImGui::GetIO().Fonts->Fonts.Size > 1 ? ImGui::GetIO().Fonts->Fonts[1] : ImGui::GetFont();
        draw->AddRectFilled(base, base + ImVec2(158.0f, 576.0f), Color(18, 21, 30, 250), 14.0f, ImDrawFlags_RoundCornersLeft);
        draw->AddRectFilled(base + ImVec2(145.0f, 0.0f), base + ImVec2(158.0f, 576.0f), Color(18, 21, 30, 250));
        draw->AddLine(base + ImVec2(158.0f, 0.0f), base + ImVec2(158.0f, 576.0f), Color(30, 33, 43));

        Text(draw, base + ImVec2(17.0f, 18.0f), Color(83, 177, 255), "NL", kTextTitle, strong);
        Text(draw, base + ImVec2(53.0f, 15.0f), Color(226, 228, 235), "Neverlose", kTextTitle, strong);
        Text(draw, base + ImVec2(53.0f, 34.0f), Color(91, 96, 108), "Counter-Strike 2", 9.0f);
        draw->AddLine(base + ImVec2(10.0f, 56.0f), base + ImVec2(147.0f, 56.0f), Color(28, 31, 41));

        struct NavigationItem { const char* icon; const char* label; int y; };
        const NavigationItem items[] = {
            { ICON_FA_CROSSHAIRS, "Rage", 84 },
            { ICON_FA_MOUSE, "Legit", 120 },
            { ICON_FA_IMAGE, "Visuals", 190 },
            { ICON_FA_LAYER_GROUP, "Inventory", 226 },
            { ICON_FA_SLIDERS_H, "Miscellaneous", 262 }
        };
        Text(draw, base + ImVec2(16.0f, 68.0f), Color(91, 96, 108), "AIMBOT", kTextCaption);
        Text(draw, base + ImVec2(16.0f, 170.0f), Color(91, 96, 108), "COMMON", kTextCaption);

        for (int index = 0; index < 5; ++index)
        {
            const ImVec2 item_position = base + ImVec2(7.0f, static_cast<float>(items[index].y));
            if (HitTarget((std::string("##nav") + std::to_string(index)).c_str(), item_position, ImVec2(140.0f, 30.0f)))
                state.navigation = index;
            const bool selected = state.navigation == index;
            const float response = Motion(ImGui::GetItemID() ^ 0x2F7168D3u,
                selected ? 1.0f : ImGui::IsItemHovered() ? 0.55f : 0.0f, 16.0f);
            if (response > 0.001f)
                draw->AddRectFilled(item_position, item_position + ImVec2(140.0f, 30.0f),
                    MixColor(Color(18, 21, 30, 0), Color(39, 43, 54), response), 6.0f);
            TextCenteredY(draw, item_position.x + 10.0f + response, item_position.y, 30.0f,
                MixColor(Color(137, 142, 153), Color(79, 132, 255), response), items[index].icon, kTextIcon);
            TextCenteredY(draw, item_position.x + 31.0f + response, item_position.y, 30.0f,
                MixColor(Color(145, 149, 159), Color(225, 227, 234), response), items[index].label, kTextBody);
        }

        const ImVec2 account = base + ImVec2(7.0f, 531.0f);
        if (HitTarget("##account", account, ImVec2(140.0f, 38.0f)))
            state.account_open = !state.account_open;
        const float account_response = Motion(ImGui::GetItemID() ^ 0x3A56C107u,
            state.account_open ? 1.0f : ImGui::IsItemHovered() ? 0.55f : 0.0f, 16.0f);
        if (account_response > 0.001f)
            draw->AddRectFilled(account, account + ImVec2(140.0f, 38.0f),
                MixColor(Color(18, 21, 30, 0), Color(39, 43, 54), account_response), 5.0f);
        if (avatar)
            draw->AddImageRounded(avatar, account + ImVec2(7.0f, 5.0f), account + ImVec2(35.0f, 33.0f), ImVec2(0, 0), ImVec2(1, 1), Color(255, 255, 255), 14.0f);
        Text(draw, account + ImVec2(43.0f, 4.0f), Color(223, 225, 232), "nuomi", kTextControl);
        Text(draw, account + ImVec2(43.0f, 20.0f), Color(111, 116, 128), "3 days left", kTextSmall);
        Chevron(draw, account + ImVec2(130.0f + account_response, 15.0f), Color(181, 185, 195));
    }

    void Toolbar(ImDrawList* draw, const ImVec2& base)
    {
        draw->AddRectFilled(base + ImVec2(158.0f, 0.0f), base + ImVec2(748.0f, 56.0f), Color(11, 13, 20, 246), 14.0f, ImDrawFlags_RoundCornersTopRight);
        draw->AddLine(base + ImVec2(158.0f, 56.0f), base + ImVec2(748.0f, 56.0f), Color(27, 30, 39));

        const ImVec2 first = base + ImVec2(168.0f, 15.0f);
        const bool first_hovered = ImGui::IsMouseHoveringRect(first, first + ImVec2(98.0f, 29.0f));
        const float first_hover = Motion(ImGui::GetID("##toolbar_profile_motion"),
            first_hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left) ? 1.0f : first_hovered ? 0.65f : 0.0f, 18.0f);
        draw->AddRectFilled(first, first + ImVec2(98.0f, 29.0f), MixColor(Color(17, 19, 27), Color(27, 33, 46), first_hover), 6.0f);
        draw->AddRect(first, first + ImVec2(98.0f, 29.0f), MixColor(Color(28, 31, 41), Color(75, 126, 255), first_hover), 6.0f);
        TextCenteredY(draw, first.x + 10.0f, first.y, 29.0f, Color(193, 196, 205), ICON_FA_SAVE, kTextSmall);
        TextCenteredY(draw, first.x + 42.0f, first.y, 29.0f, Color(183, 186, 196), "flux", kTextControl);
        Chevron(draw, first + ImVec2(82.0f, 10.0f), Color(129, 134, 145));

        const ImVec2 second = base + ImVec2(280.0f, 15.0f);
        const bool second_hovered = ImGui::IsMouseHoveringRect(second, second + ImVec2(80.0f, 29.0f));
        const float second_hover = Motion(ImGui::GetID("##toolbar_global_motion"),
            second_hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left) ? 1.0f : second_hovered ? 0.65f : 0.0f, 18.0f);
        draw->AddRectFilled(second, second + ImVec2(80.0f, 29.0f), MixColor(Color(17, 19, 27), Color(27, 33, 46), second_hover), 6.0f);
        draw->AddRect(second, second + ImVec2(80.0f, 29.0f), MixColor(Color(28, 31, 41), Color(75, 126, 255), second_hover), 6.0f);
        TextCenteredY(draw, second.x + 12.0f, second.y, 29.0f, Color(183, 186, 196), "Global", kTextControl);
        Chevron(draw, second + ImVec2(65.0f, 10.0f), Color(129, 134, 145));

        draw->AddCircle(base + ImVec2(719.0f, 29.0f), 5.0f, Color(178, 182, 192), 0, 2.0f);
        draw->AddLine(base + ImVec2(723.0f, 33.0f), base + ImVec2(727.0f, 37.0f), Color(178, 182, 192), 2.0f);
    }

    void Settings(ImDrawList* draw, const ImVec2& base)
    {
        dropdown.valid = false;
        const ImVec2 main_card = base + ImVec2(167.0f, 84.0f);
        const ImVec2 other_card = base + ImVec2(458.0f, 84.0f);
        const ImVec2 selection_card = base + ImVec2(167.0f, 340.0f);
        const ImVec2 anti_aim_card = base + ImVec2(458.0f, 340.0f);
        Card(draw, main_card, ImVec2(281.0f, 221.0f), "MAIN");
        Card(draw, other_card, ImVec2(277.0f, 221.0f), "OTHER");
        Card(draw, selection_card, ImVec2(281.0f, 221.0f), "SELECTION");
        Card(draw, anti_aim_card, ImVec2(277.0f, 221.0f), "ANTI-AIM");

        ImGui::BeginDisabled(state.dropdown_open);
        ToggleRow(draw, main_card, 281.0f, 0, "##enabled", "Enabled", state.enabled, true);
        ToggleRow(draw, main_card, 281.0f, 1, "##silent", "Silent Aim", state.silent_aim);
        ToggleRow(draw, main_card, 281.0f, 2, "##automatic", "Automatic Fire", state.automatic_fire);
        ToggleRow(draw, main_card, 281.0f, 3, "##walls", "Aim Through Walls", state.aim_through_walls);
        SelectRow(draw, main_card, 281.0f, 4, 0, "Refine Shot", state.refine_shot,
            kRefineOptions, IM_ARRAYSIZE(kRefineOptions));
        SliderRow(draw, main_card, 281.0f, 5, "##fov", "Field of View", state.field_of_view, 180.0f, "%.1f\xC2\xB0");

        SelectRow(draw, other_card, 277.0f, 0, 1, "History", state.history,
            kHistoryOptions, IM_ARRAYSIZE(kHistoryOptions));
        SelectRow(draw, other_card, 277.0f, 1, 2, "Delay Shot", state.delay_shot,
            kDelayOptions, IM_ARRAYSIZE(kDelayOptions), true);
        SelectRow(draw, other_card, 277.0f, 2, 3, "Remove Spread", state.remove_spread,
            kSpreadOptions, IM_ARRAYSIZE(kSpreadOptions));
        ToggleRow(draw, other_card, 277.0f, 3, "##duck", "Duck Peek Assist", state.duck_peek_assist);
        ToggleRow(draw, other_card, 277.0f, 4, "##quick", "Quick Peek Assist", state.quick_peek_assist, true);
        ToggleRow(draw, other_card, 277.0f, 5, "##double", "Double Tap", state.double_tap);

        SelectRow(draw, selection_card, 281.0f, 0, 4, "Hitbox", state.hitbox,
            kHitboxOptions, IM_ARRAYSIZE(kHitboxOptions));
        SelectRow(draw, selection_card, 281.0f, 1, 5, "Multipoint", state.multipoint,
            kMultipointOptions, IM_ARRAYSIZE(kMultipointOptions));
        SliderRow(draw, selection_card, 281.0f, 2, "##hitchance", "Hitchance", state.hitchance, 100.0f, "%.0f%%");
        SliderRow(draw, selection_card, 281.0f, 3, "##mindamage", "Minimum Damage", state.minimum_damage, 100.0f, "%.0f");
        ToggleRow(draw, selection_card, 281.0f, 4, "##safe", "Safe Points", state.safe_points, true);
        ToggleRow(draw, selection_card, 281.0f, 5, "##body", "Body Aim", state.body_aim);

        ToggleRow(draw, anti_aim_card, 277.0f, 0, "##antiaim", "Enabled", state.anti_aim, true);
        ToggleRow(draw, anti_aim_card, 277.0f, 1, "##breathing", "Suppress Breathing Animations", state.suppress_breathing);
        SelectRow(draw, anti_aim_card, 277.0f, 2, 6, "Leg Movement", state.leg_movement,
            kLegOptions, IM_ARRAYSIZE(kLegOptions));
        ChevronRow(draw, anti_aim_card, 277.0f, 3, "Pitch");
        ChevronRow(draw, anti_aim_card, 277.0f, 4, "Yaw");
        ChevronRow(draw, anti_aim_card, 277.0f, 5, "Mouse Override");
        ImGui::EndDisabled();
    }

    void PopoverRow(ImDrawList* draw, const ImVec2& panel, int row, const char* icon, const char* label, const char* value = nullptr)
    {
        const float y = panel.y + kPopoverRowsY + row * kPopoverRowHeight;
        TextCenteredY(draw, panel.x + 21.0f, y, kPopoverRowHeight, Color(184, 187, 197), icon, kTextIcon);
        TextCenteredY(draw, panel.x + 49.0f, y, kPopoverRowHeight, Color(176, 179, 189), label, kTextControl);
        if (value)
        {
            const float value_width = ImGui::GetFont()->CalcTextSizeA(kTextControl, FLT_MAX, 0.0f, value).x;
            TextCenteredY(draw, panel.x + 187.0f - value_width, y, kPopoverRowHeight, Color(155, 159, 170), value, kTextControl);
            Chevron(draw, ImVec2(panel.x + 194.0f, y + 9.0f), Color(159, 163, 174));
        }
    }

    void AccountPopover(ImDrawList* draw, const ImVec2& base, ID3D11ShaderResourceView* avatar)
    {
        const float open = Motion(ImGui::GetID("##account_popover_motion"), state.account_open ? 1.0f : 0.0f, 15.0f, 0.0f);
        if (open <= 0.001f)
            return;

        const ImVec2 panel = base + ImVec2(151.0f, 349.0f);
        const int first_vertex = draw->VtxBuffer.Size;
        draw->AddRectFilled(panel + ImVec2(0.0f, 5.0f), panel + ImVec2(215.0f, 399.0f), Color(0, 0, 0, 80), 14.0f);
        draw->AddRectFilled(panel, panel + ImVec2(215.0f, 395.0f), Color(25, 25, 34, 252), 14.0f);
        draw->AddRect(panel, panel + ImVec2(215.0f, 395.0f), Color(37, 39, 50), 14.0f);

        if (avatar)
            draw->AddImageRounded(avatar, panel + ImVec2(19.0f, 17.0f), panel + ImVec2(58.0f, 56.0f), ImVec2(0, 0), ImVec2(1, 1), Color(255, 255, 255), 20.0f);
        Text(draw, panel + ImVec2(67.0f, 15.0f), Color(226, 228, 235), "nuomi", kTextBody);
        Text(draw, panel + ImVec2(67.0f, 33.0f), Color(187, 190, 201), "Till: July 16 2026", kTextControl);
        Text(draw, panel + ImVec2(67.0f, 50.0f), Color(78, 129, 255), "Renew", kTextControl);
        draw->AddLine(panel + ImVec2(14.0f, 72.0f), panel + ImVec2(201.0f, 72.0f), Color(36, 38, 48));

        PopoverRow(draw, panel, 0, ICON_FA_GLOBE, "Language", "1");
        PopoverRow(draw, panel, 1, ICON_FA_TEXT_SIZE, "Menu Scale", "Auto");
        PopoverRow(draw, panel, 2, ICON_FA_EYE, "ESP Scale", "Auto");
        PopoverRow(draw, panel, 3, ICON_FA_WINDOW_RESTORE, "Windows Scale", "Auto");
        PopoverRow(draw, panel, 4, ICON_FA_RULER, "Units", "Auto");
        PopoverRow(draw, panel, 5, ICON_FA_PAINT_BRUSH, "Style");
        draw->AddCircle(panel + ImVec2(165.0f, kPopoverRowsY + 5 * kPopoverRowHeight + 12.0f), 5.0f, Color(166, 171, 181), 8, 1.2f);
        draw->AddRectFilled(panel + ImVec2(182.0f, kPopoverRowsY + 5 * kPopoverRowHeight + 6.0f), panel + ImVec2(197.0f, kPopoverRowsY + 5 * kPopoverRowHeight + 21.0f), Color(115, 214, 210), 4.0f);
        PopoverRow(draw, panel, 6, ICON_FA_SHIELD_ALT, "Safe Mode", "Automatic");
        PopoverRow(draw, panel, 7, ICON_FA_SYNC_ALT, "Synchronization");
        ImGui::BeginDisabled(open < 0.85f);
        Toggle(draw, "##sync", panel + ImVec2(168.0f, kPopoverRowsY + 7 * kPopoverRowHeight + 5.0f), state.synchronization);
        ImGui::EndDisabled();
        PopoverRow(draw, panel, 8, ICON_FA_INFO_CIRCLE, "About");
        PopoverRow(draw, panel, 9, ICON_FA_COMMENT_ALT, "Chat");

        const float eased = 1.0f - std::pow(1.0f - open, 3.0f);
        const ImVec2 pivot = panel + ImVec2(107.5f, 16.0f);
        const float scale = ImLerp(0.96f, 1.0f, eased);
        for (int index = first_vertex; index < draw->VtxBuffer.Size; ++index)
        {
            ImDrawVert& vertex = draw->VtxBuffer[index];
            vertex.pos = pivot + (vertex.pos - pivot) * scale + ImVec2(0.0f, (1.0f - eased) * 9.0f);
            const ImU32 alpha = (vertex.col >> IM_COL32_A_SHIFT) & 0xFFu;
            vertex.col = (vertex.col & 0x00FFFFFFu) |
                (static_cast<ImU32>(alpha * eased) << IM_COL32_A_SHIFT);
        }
    }
}

void RenderNeverloseMenu(ID3D11ShaderResourceView* avatar, float reveal_progress)
{
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(kCanvasSize, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus;
    const float progress = ImClamp(reveal_progress, 0.0f, 1.0f);
    if (progress < 1.0f)
        flags |= ImGuiWindowFlags_NoInputs;

    if (ImGui::Begin("Neverlose Menu", nullptr, flags))
    {
        const ImVec2 base = ImGui::GetWindowPos();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const int first_vertex = draw->VtxBuffer.Size;
        draw->AddRectFilled(base, base + kShellSize, Color(13, 15, 22, 248), 14.0f);
        Sidebar(draw, base, avatar);
        Toolbar(draw, base);
        Settings(draw, base);
        AccountPopover(draw, base, avatar);
        DropdownPopup(draw);

        if (progress < 1.0f)
        {
            const float eased = 1.0f - std::pow(1.0f - progress, 4.0f);
            const float scale = ImLerp(0.92f, 1.0f, eased);
            const ImVec2 pivot = base + kCanvasSize * 0.5f;
            const ImVec2 translation(0.0f, (1.0f - eased) * 16.0f);
            for (int index = first_vertex; index < draw->VtxBuffer.Size; ++index)
            {
                ImDrawVert& vertex = draw->VtxBuffer[index];
                vertex.pos = pivot + (vertex.pos - pivot) * scale + translation;
                const ImU32 alpha = (vertex.col >> IM_COL32_A_SHIFT) & 0xFFu;
                vertex.col = (vertex.col & 0x00FFFFFFu) |
                    (static_cast<ImU32>(alpha * eased) << IM_COL32_A_SHIFT);
            }
        }
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}
