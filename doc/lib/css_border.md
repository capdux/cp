# `cap:css_border`

纯 CP 实现的 CSS Border 属性解析与规范化序列化库，吸收自 `/data/dev/bun/src/css/properties/border.cp`。

## 导出类型

- `LineStyle`：`none | hidden | inset | groove | outset | ridge | dotted | dashed | solid | double`
- `BorderLengthUnit`：`px | em | rem | pt | vw | vh`
- `BorderSideWidthKind` / `BorderSideWidth`：`thin | medium | thick | <non-negative length>`
- `BorderColorKind` / `BorderColorValue`：`currentcolor | transparent | #rgb/#rgba/#rrggbb/#rrggbbaa | 常见命名色`
- `Border`：`border` / `border-top` / `border-inline` 简写（支持任意顺序 `<line-width> || <line-style> || <color>` 解析，序列化时自动省略默认值，全默认时输出 `none`）
- `BorderWidth` / `BorderStyle` / `BorderColor`：1~4 值四边简写解析与最短 CSS 压缩序列化
- `BorderLogicalWidth`：`border-block-width` / `border-inline-width` 1~2 值逻辑边简写

## 使用示例

```ts
import { Border, BorderWidth, BorderStyle, BorderColor } from "cap:css_border";

let buf: [64]u8 = [0; 64];
const b = Border.parse("#ff0000 solid 2px") ?? unreachable;
const css = b.toCss(buf[0..]); // "2px solid #ff0000"
```
