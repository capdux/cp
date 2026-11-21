# `cap:css_time` — CSS `<time>` 时间值解析、运算与最短规范序列化

源自 Bun `css/values/time.cp`，纯 CP 零堆分配实现 CSS `<time>`（`s` / `ms`）大小写无关解析、单位转换、保单位加减乘、跨单位比较及最短 CSS 序列化（`0.05s`→`50ms`、`500ms`→`0.5s`、`0ms`→`0s`）。

## 导出符号

- `TimeUnit`（`seconds` / `milliseconds`）
- `CssTime`: `seconds` / `milliseconds` / `parse` / `isZero` / `toMs` / `toSeconds` / `add` / `sub` / `mul` / `eql` / `cmp` / `toCss`
