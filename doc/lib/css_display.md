# `cap:css_display` — CSS Display Level 3 与 Visibility 属性模型

源自 Bun `css/properties/display.cp`，纯 CP 零依赖实现 CSS Display Level 3 单值关键字、内外流多关键字组合（如 `inline flow-root`、`block flex`、`inline list-item`）、旧版简写（`inline-block`、`inline-flex`、`inline-grid` 及 `-webkit-`/`-ms-`/`-moz-` 前缀）与最短规范化 CSS 序列化。

## 导出符号

- `Visibility`（`visible` / `hidden` / `collapse`）
- `DisplayKeyword`（14 种内部/盒生成关键字 + `isTableInternal`/`isRubyInternal`）
- `DisplayOutside`（`block` / `inline_box` / `run_in`）
- `DisplayInside`（`flow` / `flow_root` / `table` / `flex` / `grid` / `ruby` 及厂商前缀）
- `DisplayPair`、`DisplayKind`、`Display`（`parse` / `isNone` / `toCssBuf`）
