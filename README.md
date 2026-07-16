<div align="center">

# Neverlose Last

**A high-fidelity Direct3D 11 + Dear ImGui menu inspired by the visual language of the neverlose.cc CS2 V4 product.**

**基于 Direct3D 11 与 Dear ImGui 构建，视觉设计参考 neverlose.cc CS2 V4 产品的高还原度菜单界面。**

[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![DirectX 11](https://img.shields.io/badge/DirectX-11-107C10?logo=windows&logoColor=white)](https://learn.microsoft.com/windows/win32/direct3d11/)
[![Dear ImGui](https://img.shields.io/badge/UI-Dear%20ImGui-1F6FEB)](https://github.com/ocornut/imgui)
[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](https://www.gnu.org/licenses/agpl-3.0.html)

[English](#english) · [简体中文](#简体中文) · [Showcase / 效果展示](#showcase--效果展示)

</div>

> [!NOTE]
> This is an independent UI recreation intended for learning, research, and interface experimentation. It is not affiliated with, endorsed by, or sponsored by neverlose.cc.
>
> 本项目是用于学习、研究与界面实验的独立 UI 复刻作品，与 neverlose.cc 不存在隶属、授权、赞助或官方合作关系。

## Showcase / 效果展示

| Main / 主界面 | Color Picker / 颜色选择器 |
| --- | --- |
| [![Main menu](https://i.imgur.com/tOQlWwy.png)](https://imgur.com/tOQlWwy) | [![Color picker](https://i.imgur.com/uLqjMpg.png)](https://imgur.com/uLqjMpg) |

| Config / 配置菜单 | Profile / 个人资料 |
| --- | --- |
| [![Config menu](https://i.imgur.com/L1z2N1T.png)](https://imgur.com/L1z2N1T) | [![Profile menu](https://i.imgur.com/PonKoBv.png)](https://imgur.com/PonKoBv) |

---

## English

### Overview

Neverlose Last is a standalone Win32 menu showcase built with C++17, Direct3D 11, and Dear ImGui. It focuses on polished rendering, fluid interaction, and a highly faithful recreation of the visual style used by the neverlose.cc CS2 V4 product.

### Features

- GPU-accelerated Gaussian blur powered by Direct3D 11.
- High-fidelity visual recreation targeting more than 95% similarity to the reference interface.
- Elegant shadows created from layered, linearly blended rectangles.
- Adjustable menu DPI scaling for different display densities.
- Crisp text rasterization with carefully tuned font rendering.
- Precise rounded-corner rendering and clipping throughout the interface.
- Pill-shaped toggle switches driven by smooth linear animations.
- Editable sliders with transition animations, implemented as templates with both `int` and `float` support.
- Single-select and multi-select combo components.
- Highly customizable secondary combo menus with a glass-like combination of Gaussian blur, translucent backgrounds, and shadows.
- Clean iconography based on the Font Awesome character set.
- A polished rounded-rectangle saturation/value color field with mouse-drag interaction.
- Hue and alpha sliders with real-time color and transparency previews.
- A flexible secondary color-picker menu with optional entries and extensive styling controls.
- Support for editing multiple colors from a single color-picker popup.
- A cinematic startup animation.
- Faithfully recreated Config and Profile secondary menus.
- One globally controlled accent color for consistent theming across the entire menu.

### Build

1. Open [`examples/imgui_examples.sln`](examples/imgui_examples.sln) in Visual Studio.
2. Select the `example_win32_directx11` project and an `x64` Debug or Release configuration.
3. Build and run the project.

The checked-in project targets C++17, the Windows 10 SDK, and the MSVC `v145` toolset. If your Visual Studio installation uses a different toolset, retarget the solution before building.

### License

Unless otherwise stated, the original project-specific code in this repository is distributed under the [GNU Affero General Public License v3.0](https://www.gnu.org/licenses/agpl-3.0.html). If you modify the software and make it available to users over a network, you must also make the corresponding source code available under the same license.

Bundled third-party code, fonts, libraries, and assets remain subject to their respective licenses. Dear ImGui retains its MIT license; see [`LICENSE.txt`](LICENSE.txt).

---

## 简体中文

### 项目简介

Neverlose Last 是一个使用 C++17、Direct3D 11 与 Dear ImGui 构建的独立 Win32 菜单展示项目。项目重点打磨渲染质感、交互动画与组件细节，并对 neverlose.cc CS2 V4 产品的界面视觉进行高还原度复刻。

### 主要特性

- 基于 Direct3D 11 GPU 渲染的高斯模糊效果。
- 高度还原参考界面的视觉细节，目标相似度超过 95%。
- 通过多层矩形线性叠加实现自然、优雅的阴影效果。
- 支持菜单 DPI 缩放，可适配不同显示密度。
- 经过精细调校的文字栅格化渲染，字体显示清晰细腻。
- 对圆角绘制、裁剪与边缘衔接进行了细致处理。
- 基于线性动画的胶囊形 Toggle 开关。
- 带输入能力和过渡动画的滑动条，采用模板函数构建，同时支持 `int` 与 `float`。
- 支持单选与多选的 Combo 组件。
- 高度可定制的二级 Combo 菜单，结合高斯模糊、半透明背景色与阴影，呈现伪玻璃质感。
- 基于 Font Awesome 字符集的精致图标系统。
- 美观的圆角矩形 SV（饱和度 / 明度）鼠标拖动选择区。
- 带实时颜色预览的色相滑动条，以及带实时透明度预览的 Alpha 滑动条。
- 高度可定制的二级 Color Picker 菜单，并支持添加可选项。
- 支持在同一个二级菜单中集中调整多种颜色。
- 精心设计的开屏动画。
- 高度还原的 Config 二级菜单与精美的 Profile 二级菜单。
- 支持统一控制全局菜单强调色，保持整体主题一致。

### 构建方式

1. 使用 Visual Studio 打开 [`examples/imgui_examples.sln`](examples/imgui_examples.sln)。
2. 选择 `example_win32_directx11` 项目，并切换到 `x64` 的 Debug 或 Release 配置。
3. 编译并运行项目。

当前工程使用 C++17、Windows 10 SDK 与 MSVC `v145` 工具集。如果你的 Visual Studio 未安装对应工具集，请先对解决方案进行重定向。

### 开源协议

除非另有说明，本仓库中的项目自有代码遵循 [GNU Affero General Public License v3.0](https://www.gnu.org/licenses/agpl-3.0.html)。如果你修改本项目并通过网络向用户提供服务，必须按照同一协议向这些用户提供对应的完整源代码。

仓库中包含的第三方代码、字体、库与资源继续遵循各自原有的许可证。Dear ImGui 仍采用 MIT 许可证，详情请参阅 [`LICENSE.txt`](LICENSE.txt)。
