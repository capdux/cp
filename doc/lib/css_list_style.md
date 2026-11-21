# `cap:css_list_style`

纯 CP 零堆分配 CSS Lists & Counters Level 3 属性库，吸收自 `/data/dev/bun/src/css/properties/list.cp`。

## 导出类型

- `PredefinedCounterStyle`：覆盖 18 种标准预定义计数器样式（`disc`/`circle`/`square`/`decimal`/`lower-roman`/`upper-alpha`/`cjk-decimal` 等）
- `ListStylePosition`：`outside | inside`
- `MarkerSide`：`match-self | match-parent`
- `ListStyleTypeKind` / `ListStyleType`：`none` / 预定义样式 / 字符串标记 / 自定义计数器名
- `ListStyle`：`list-style` 简写（支持任意顺序解析、双 `none` 消歧与默认值省略 `toCss` 序列化）
