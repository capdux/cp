# @cap/num-rational

Zero-allocation exact rational number (`Ratio64`) arithmetic, GCD reduction, comparison, rounding, and parsing engine for CP — clean-room MIT parity with Rust `num-rational`.

零堆分配的 CP 精确有理数分数引擎（`Ratio64`），始终保持最简分数与分母恒正，四则运算内置先约分防中间溢出，适用于金融汇率、音视频帧率（`30000/1001`）与无漂移网格计算（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/num-rational
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `Ratio64.new` | `(numer: i64, denom: i64) -> Ratio64?` | 自动 GCD 约分并将负号移至分子（`denom == 0` 返回 `null`） |
| `Ratio64.fromInt` / `zero` / `one` | 快捷构造器 | 构造整数、0、1 有理数 |
| `add` / `sub` / `mul` / `div` / `neg` / `abs` / `recip` | 精确四则运算 | 交叉约分防溢出，`div`/`recip` 除零返回 `null` |
| `equals` / `cmp` / `isInteger` / `floor` / `ceil` / `trunc` / `toF64` | 比较与取整 | 精确排序与正负数向零/向下/向上取整 |
| `parseRatio` | `(s: string) -> Ratio64?` | 解析 `"3/4"`, `"-6/8"`, `"42"` |
| `formatRatio` | `(r: Ratio64, out: []u8) -> string` | 格式化输出（分母为 1 时输出整数如 `"5"`，否则 `"2/3"`） |

## Usage / 使用示例

```cp
import { Ratio64, parseRatio, formatRatio } from "@cap/num-rational";

const a = parseRatio("1/3").?;
const b = parseRatio("1/6").?;
const sum = a.add(b); // exact 1/2

let buf: [64]u8 = undefined;
const text = formatRatio(sum, buf[0..64]); // "1/2"
```

## License

MIT
