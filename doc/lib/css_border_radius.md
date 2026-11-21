# `cap:css_border_radius` — CSS `border-radius` 椭圆圆角简写解析与最短序列化

源自 Bun `css/properties/border_radius.cp`，纯 CP 零堆分配实现 `border-radius` 1–4 值与 `/` 椭圆双轴简写展开、等价归约最短 CSS 输出与负半径/畸形语法拦截。

## 导出符号

- `RadiusValue`、`CornerRadius`、`BorderRadius`（`parse` / `toCss` / `isUniform` / `isCircular` / `eql`）
