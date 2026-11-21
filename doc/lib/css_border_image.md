# `cap:css_border_image` — CSS Backgrounds 3 `border-image` 属性族解析与序列化

> 溯源：从 `/data/dev/bun/src/css/properties/border_image.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:css_border_image` 提供零外部依赖、零堆分配的 CSS Backgrounds 3 `border-image` 完整属性族解析与规范化序列化：

- `BorderImageRepeatKeyword` & `BorderImageRepeat`（`stretch` / `repeat` / `round` / `space`，单/双轴压缩）；
- `SliceOffset` & `BorderImageSlice`（1~4 边非负无单位数或百分比 + 前置/后置 `fill`）；
- `BorderImageSideWidth` & `BorderImageWidth`（`auto` / 非负无单位数 / `px` / `%`，1~4 边压缩）；
- `BorderImageOutsetValue` & `BorderImageOutset`（非负无单位数 / `px`，1~4 边压缩）；
- `BorderImage` 完整简写（支持任意顶层顺序、双斜杠 `slice / width / outset` 与 `slice / / outset` 省略、默认值规范化省略 `toCss`）。

---

## 快速导入

```cp
import {
    BorderImage,
    BorderImageSlice,
    SliceOffset,
    BorderImageWidth,
    BorderImageSideWidth,
    WidthKind,
    BorderImageOutset,
    BorderImageOutsetValue,
    OutsetKind,
    BorderImageRepeat,
    BorderImageRepeatKeyword,
} from "cap:css_border_image";
```

---

## API 参考

| 结构体 / 枚举 | 核心方法 | 默认值 |
| :--- | :--- | :--- |
| `BorderImageRepeatKeyword` | `fromStr(s)`, `asStr()` | `stretch` |
| `BorderImageRepeat` | `defaultRepeat()`, `parse(s)`, `isDefault()`, `eql(other)`, `toCss(out)` | `stretch stretch` |
| `SliceOffset` | `percent(v)`, `number(v)`, `parse(tok)`, `eql(other)` | `100%` |
| `BorderImageSlice` | `defaultSlice()`, `parse(s)`, `isDefault()`, `eql(other)`, `toCss(out)` | `100%` (`fill = false`) |
| `BorderImageSideWidth` / `BorderImageWidth` | `defaultWidth()`, `parse(s)`, `isDefault()`, `eql(other)`, `toCss(out)` | `1` |
| `BorderImageOutsetValue` / `BorderImageOutset` | `defaultOutset()`, `parse(s)`, `isDefault()`, `eql(other)`, `toCss(out)` | `0` |
| `BorderImage` | `parse(input): BorderImage?`, `toCss(out: []u8): []const u8` | `source = "none"` + 以上默认 |

---

## 使用示例

```cp
import { BorderImage } from "cap:css_border_image";

fn demo() {
    if (bi := BorderImage.parse("url(border.png) 30 fill / 10px 20px / 5px round space")) {
        let buf: [128]u8 = [0; 128];
        const css = bi.toCss(buf[0..128]);
        * = css;
    }
}
```
