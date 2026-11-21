# `cap:css_ui` — CSS Basic User Interface Level 4 属性集

源自 Bun `css/properties/ui.cp`，纯 CP 零堆分配实现 `color-scheme`（`normal`/`light`/`dark`/`only`）、`resize`、`user-select`、`caret-shape`、`appearance`（16 种标准与 compat-auto 变体）及 36 种标准 `cursor` 光标关键字解析与序列化。

## 导出符号

- `ColorScheme`: `normal` / `isNormal` / `parse` / `toCss` / `eql`
- `Resize`、`UserSelect`、`CaretShape`
- `Appearance`（附 `isCompatAuto()`）
- `CursorKeyword`（36 种标准光标，附 `isResize()`）
