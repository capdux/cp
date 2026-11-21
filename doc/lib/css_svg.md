# `cap:css_svg` — SVG 2 CSS 渲染属性族解析与规范序列化

> 溯源：从 `/data/dev/bun/src/css/properties/svg.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:css_svg` 提供零外部依赖、零堆分配的 SVG 2 CSS 渲染属性族解析与规范序列化：

- `SVGPaintKind` / `SVGPaintFallbackKind` / `SVGPaint`（`none` / 颜色 / `url(#id) [fallback]` / `context-fill` / `context-stroke`）；
- `StrokeLinecap`、`StrokeLinejoin`、`StrokeDasharray`（最多 16 项非负长度/百分比/无单位数）；
- `PaintOrderItem` / `PaintOrder`（`normal` 或 1~3 项无重复排列，自动补全并按最短形式规范压缩）；
- `ColorInterpolation`、`ColorRendering`、`ShapeRendering`、`TextRendering`、`ImageRendering` 五大渲染提示枚举。

---

## 快速导入

```cp
import {
    SVGPaint,
    SVGPaintKind,
    SVGPaintFallbackKind,
    StrokeLinecap,
    StrokeLinejoin,
    StrokeDasharray,
    DashEntry,
    DashUnit,
    PaintOrder,
    PaintOrderItem,
    ColorInterpolation,
    ColorRendering,
    ShapeRendering,
    TextRendering,
    ImageRendering,
} from "cap:css_svg";
```

---

## API 参考

| 符号 | 核心方法 | 说明 |
| :--- | :--- | :--- |
| `SVGPaint` | `parse(input): SVGPaint?`, `toCss(out)` | 支持 `none`, `context-fill`, `context-stroke`, 颜色, `url(#id) [none \| color]` |
| `StrokeLinecap` | `fromStr(s)`, `asStr()` | `butt`, `round`, `square` |
| `StrokeLinejoin` | `fromStr(s)`, `asStr()` | `miter`, `round`, `bevel`, `miter-clip`, `arcs` |
| `StrokeDasharray` | `none()`, `parse(s)`, `slice()`, `toCss(out)` | `none` 或 1..16 项非负数字/`px`/`%`（逗号或空白分隔） |
| `PaintOrder` | `normal()`, `parse(s)`, `isNormal()`, `eql(other)`, `toCss(out)` | 自动补全缺失项并压缩为最短规范形式（`fill stroke markers` -> `"normal"`） |
| 5 大渲染提示枚举 | `fromStr(s)`, `asStr()` | `ColorInterpolation`, `ColorRendering`, `ShapeRendering`, `TextRendering`, `ImageRendering` |

---

## 使用示例

```cp
import { SVGPaint, PaintOrder, StrokeDasharray } from "cap:css_svg";

fn demo() {
    let buf: [64]u8 = [0; 64];
    if (p := SVGPaint.parse("url(#grad) red")) {
        * = p.toCss(buf[0..64]); // "url(#grad) red"
    }
    if (po := PaintOrder.parse("stroke fill markers")) {
        * = po.toCss(buf[0..64]); // "stroke"
    }
    if (da := StrokeDasharray.parse("4px, 2px, 10%")) {
        * = da.toCss(buf[0..64]); // "4px 2px 10%"
    }
}
```
