# Neverlose Menu Design Contract

## 1. Reference and intent

- Exact visual reference: `C:\Users\XPTNo\AppData\Local\Temp\codex-clipboard-7975bf79-bc5d-4160-95ae-61207082eff8.png`.
- Surface: standalone Win32 + DirectX 11 Dear ImGui preview.
- Goal: reproduce the reference's dense dark settings panel with a left navigation rail, top context controls, two-column setting cards, and an expanded account popover.
- The implementation must remain live ImGui UI. The reference image must never be rendered as a UI texture.

## 2. Canvas and layout

- Preview canvas: `750 x 756` logical pixels.
- Main shell: `(0, 0)`, `748 x 576`, corner radius `14`.
- Sidebar: width `158`; toolbar: `56` high.
- Content starts at `x=167`; columns start at `167` and `458`.
- Column widths: `281` and `277`; gutter: `10`.
- Top cards: `y=84`, `h=221`; lower cards: `y=340`, `h=221`.
- Account popover: `(151, 349)`, `215 x 395`, radius `14`, drawn above the shell.

## 3. Color tokens

- `canvas-overlay`: `#090B12` at 96% opacity.
- `shell`: `#0D0F16` at 97% opacity.
- `sidebar`: `#12151E` at 98% opacity.
- `toolbar`: `#0B0D14` at 96% opacity.
- `card`: `#11131B`.
- `control`: `#181B25`.
- `selected`: `#272B36`.
- `border`: `#20232E`.
- `text`: `#D9DBE2`.
- `text-muted`: `#9296A2`.
- `text-faint`: `#5E6370`.
- `accent`: `#4E83FF`.
- `accent-bright`: `#66A4FF`.
- `toggle-off`: `#84909C`.
- `style-chip`: `#73D6D2`.

## 4. Typography

- Primary face: embedded Museo 500 at `13px`.
- Strong labels: embedded Museo 900 at `14px`.
- Section eyebrow: `10px`, uppercase, faint.
- Row labels: `13px`; secondary/account text: `12px`.
- Line heights are compact and vertically centered in `37px` rows.

## 5. Primitive anatomy

- Navigation item: `140 x 30`, radius `6`; selected item uses `selected` fill, accent icon, and bright text.
- Settings card: radius `13`, one-pixel border, rows separated by one-pixel dividers.
- Toggle: `29 x 18`; circular `14px` thumb; accent fill when enabled.
- Select: `126 x 23`, radius `5`, muted value, trailing chevron.
- Slider: `126 x 4` track, accent progress, `10px` white thumb, compact value pill.
- Account bar: avatar, two-line identity, chevron; click toggles the account popover.
- Account popover: identity header, divider, eight compact menu rows, two toggles, one style color chip.

## 6. Interaction states

- Navigation selection updates on click.
- Toggle controls flip immediately and preserve state for the process lifetime.
- Account bar opens and closes the popover; it starts open to match the supplied reference state.
- Hover uses a subtle `selected` overlay only on actionable rows.
- Focus/navigation remain enabled through Dear ImGui keyboard navigation.

## 7. Accessibility constraints

- All text/background pairs maintain high contrast in the dark palette.
- Click targets are at least `29 x 30` even when the visible toggle is smaller.
- State is conveyed through both color and thumb position.
- No information is encoded solely in decorative imagery.

## 8. Accepted debt

- The standalone preview uses the host wallpaper instead of a live Counter-Strike scene.
- Embedded Museo replaces the reference application's exact proprietary font.
- The supplied Font Awesome 5 Pro Solid font provides menu glyphs; simple vector marks remain only for custom control geometry.

## 9. HexSync startup motion

- Reference: `C:\Users\XPTNo\Desktop\Code\HexSync logo\HexSync Logo.html` and its frame-accurate renderer `exp_render.js`.
- Surface: a full-canvas ImGui startup overlay rendered above the menu, not a GIF or video texture.
- Timeline: the mark begins at `0.2s`, locks by `1.7s`, the wordmark enters from `2.1s`, and the overlay hands off to the menu after `3.4s`.
- Geometry: the mark rotates from `-120deg` while scaling from `0.38` to `1`; a six-sided containment ring draws clockwise while rotating from `60deg` to `0deg`.
- Effects: cyan radial halo, lock flash, two connector-node pings, a low-opacity counter-rotating ghost ring, and a subtle post-lock breath.
- Typography: `HexSync` is drawn one glyph at a time; `Hex` uses the foreground token and `Sync` interpolates from `#43B8EC` to `#1F74D4`.
- Interaction: `F5` replays the startup sequence without restarting the process; `Esc` skips immediately to the live menu.
- Accessibility: motion is finite during startup, the persistent breath is removed when the overlay hands off, and skipping never blocks menu input.
- Performance: all geometry is generated directly into `ImDrawList`; only the transparent logo is a texture, with no per-frame allocations proportional to screen size.
