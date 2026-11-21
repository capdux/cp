# `cap:css_box_shadow` — CSS `box-shadow` 与 `border`/`outline` 线型属性

源自 Bun `css/properties/{box_shadow,outline}.cp`，纯 CP 零堆分配实现 `box-shadow` 顺序无关简写解析与最短规范序列化（自动省略零 blur/spread 与默认 `currentcolor`），以及 10 种 `LineStyle` / `OutlineStyle` 线型枚举。

## 导出符号

- `LineStyle`（`none`/`hidden`/`inset`/`groove`/`outset`/`ridge`/`dotted`/`dashed`/`solid`/`double`，附 `isVisible()`）
- `OutlineStyle`（`auto` + 9 种线型）
- `ShadowLength`、`BoxShadow`（`parse` / `toCss`）
