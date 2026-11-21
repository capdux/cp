# `cap:css_animation` — CSS Animations & Transitions 简写属性解析与序列化

源自 Bun `css/properties/{animation,transition}.cp`，组合 `cap:css_time` 与 `cap:css_easing`，纯 CP 零堆分配实现 CSS `animation` 与 `transition` 顺序无关简写解析、默认值化简与规范化 CSS 输出。

## 导出符号

- `AnimationDirection`、`AnimationPlayState`、`AnimationFillMode`、`IterationCount`
- `Animation`: `parse` / `toCss`
- `Transition`: `parse` / `toCss`
