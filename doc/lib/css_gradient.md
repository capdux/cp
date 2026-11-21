# `cap:css_gradient` — CSS Images 4 渐变解析与规范序列化

> 溯源：从 `/data/dev/bun/src/css/values/gradient.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:css_gradient` 提供零外部依赖、零堆分配的 CSS Images 4 渐变解析与规范序列化引擎：

- 支持全部 6 种标准渐变：`linear-gradient`、`repeating-linear-gradient`、`radial-gradient`、`repeating-radial-gradient`、`conic-gradient`、`repeating-conic-gradient`；
- 支持角度（`deg`/`rad`/`grad`/`turn`）与 8 方向 `to <side-or-corner>`、径向形状/尺寸/定位、锥形 `from <angle> at <position>` 及 CSS Images 4 双位置色标（如 `red 10% 30%`）与 `rgba(...)` 带括号颜色；
- `toCss(out)` 自动省略默认值（`to bottom` / `180deg`、`ellipse farthest-corner`、`from 0deg`、`at center`）。

---

## 快速导入

```cp
import {
    Gradient,
    GradientKind,
    DirectionSide,
    LineDirection,
    RadialShape,
    RadialExtent,
    ColorStop,
    LinearGradient,
    RadialGradient,
    ConicGradient,
    MAX_STOPS,
} from "cap:css_gradient";
```

---

## API 参考

| 符号 | 核心成员 / 方法 | 说明 |
| :--- | :--- | :--- |
| `GradientKind` | `linear`, `repeating_linear`, `radial`, `repeating_radial`, `conic`, `repeating_conic`; `fnName()`, `isRepeating()` | 渐变类型枚举 |
| `LineDirection` | `defaultDir()`, `fromAngle(deg)`, `fromSide(side)`, `parse(raw)`, `isDefault()`, `toDegrees()` | 线性渐变方向（角度或 8 方向） |
| `RadialShape` / `RadialExtent` | `ellipse`/`circle`; `farthest_corner`/`closest_side`/`farthest_side`/`closest_corner` | 径向渐变形状与尺寸范围 |
| `ColorStop` | `{ color, pos1, pos2 }`; `hasPos1()`, `hasPos2()`, `isDoublePosition()` | 单/双位置色标 |
| `Gradient` | `parse(input): Gradient?`, `isRepeating()`, `stopCount()`, `toCss(out: []u8): []const u8` | 统一渐变解析与规范化序列化 |

---

## 使用示例

```cp
import { Gradient } from "cap:css_gradient";

fn normalizeGradient() {
    if (g := Gradient.parse("linear-gradient(180deg, red 10% 30%, blue)")) {
        let buf: [128]u8 = [0; 128];
        const css = g.toCss(buf[0..128]); // "linear-gradient(red 10% 30%, blue)"
        * = css;
    }
}
```
