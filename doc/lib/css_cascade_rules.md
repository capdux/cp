# `cap:css_cascade_rules`

纯 CP 零堆分配 CSS Cascade 5/6 与作用域规则头解析/序列化库（`@import`、`@scope`、`@layer`），吸收自 `/data/dev/bun/src/css/rules/{import,scope,viewport}.cp`。

## 导出类型

- `ImportLayerKind` / `ImportRule`：完整解析 `@import <string|url()> [layer|layer(name)]? [supports(cond)]? [media]?` 与规范化 `toCss`
- `ScopePrelude`：解析 `@scope [(<start>)]? [to (<end>)]?`（支持隐式作用域与下界限制）
- `LayerStatement`：解析 `@layer base, components.buttons, utilities;` 声明列表与标识符校验
