# `cap:css_units` — CSS `<angle>` 与 `<resolution>` 单位解析、换算与序列化

> 溯源：从 `/data/dev/bun/src/css/values/angle.cp` 与 `/data/dev/bun/src/css/values/resolution.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:css_units` 提供零外部依赖、零堆分配的 CSS Values Level 4 `<angle>` 与 `<resolution>` 单位解析、跨单位换算、运算与规范 CSS 序列化能力。

---

## 快速导入

```cp
import {
    Angle,
    AngleUnit,
    Resolution,
    ResolutionUnit,
    Error,
} from "cap:css_units";
```

---

## API 参考

### 1. `Error`

`EmptyInput`, `InvalidNumber`, `MissingUnit`, `UnknownUnit`, `NegativeResolution`, `BufferTooSmall`

### 2. `AngleUnit` & `Angle`

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| 构造 | `zero()`, `deg(v)`, `rad(v)`, `grad(v)`, `turn(v)` | 快速构造四单位角度值 |
| `parse` | `static fn parse(s: string): Angle!` | 严格解析带单位 `<angle>`（`deg`/`rad`/`grad`/`turn`，ASCII 大小写无关） |
| `parseWithUnitlessZero` | `static fn parseWithUnitlessZero(s: string): Angle!` | 允许无单位 `0` 解析为 `0deg` |
| 跨单位换算 | `toDegrees()`, `toRadians()`, `toGradians()`, `toTurns()` | 四单位高精度 `f64` 互转 |
| `normalizeDegrees` | `fn normalizeDegrees(): f64` | 归一化到 `[0.0, 360.0)` |
| 运算与比较 | `isZero()`, `add(other)`, `sub(other)`, `scale(f)`, `eql(other)` | 同单位保单位、跨单位统一 `deg` 运算与数值等价比较 |
| 规范序列化 | `toCss(buf: []u8): string!` / `toCssWithUnitlessZero(buf: []u8): string!` | 零堆分配写入缓冲区 |

### 3. `ResolutionUnit` & `Resolution`

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| 构造 | `dpi(v)`, `dpcm(v)`, `dppx(v)` | 构造分辨率值 |
| `parse` | `static fn parse(s: string): Resolution!` | 解析 `dpi`/`dpcm`/`dppx`/`x`（`2x` -> `2dppx`，拒绝负值 `NegativeResolution`） |
| 跨单位换算 | `toDpi()`, `toDppx()`, `toDpcm()` | 按 CSS 标准 `1dppx = 96dpi`, `1in = 2.54cm` 互转 |
| 比较与序列化 | `eql(other)`, `toCss(buf)`, `toCssShort(buf)` | 数值等价比较与规范/简短（`x`）序列化 |

---

## 使用示例

```cp
import { Angle, Resolution } from "cap:css_units";

fn demo()! {
    const a = Angle.parse("0.5turn")?;
    const deg = a.toDegrees(); // 180.0
    const r = Resolution.parse("2x")?;
    const dpi = r.toDpi();     // 192.0
    * = deg;
    * = dpi;

    let buf: [32]u8 = [0; 32];
    const css_text = r.toCss(buf[0..32])?; // "2dppx"
    * = css_text;
}
```
