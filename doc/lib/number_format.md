# `cap:number_format` — 千位分组、定点数、英文序数词与多进制格式化库

> 参考来源：/data/dev/bun/src/bun_core/fmt.cp | 吸收日期：2026-11-22

## 概述

`cap:number_format` 吸收自 Bun 格式化工具体系（`src/bun_core/fmt.cp`），提供纯 CP、零外部依赖、零堆分配的整数千位分组、有符号/无符号格式化、英文序数词（`1st` / `2nd` / `3rd` / `11th`）、任意进制（`2..=36`）以及定点数小数格式化器。

## 行为契约与边界规则

- **千位分组与有符号极值安全**：
  - `formatGroupedU64` / `formatGroupedI64` 支持自定义分隔符（如 `','`、`'_'`、`' '`）；
  - `formatGroupedI64` 正确处理 `i64::MIN`（`-9,223,372,036,854,775,808`），避免直接取负导致的有符号整数溢出。
- **英文序数词特例（`ordinalSuffix` / `formatOrdinal`）**：严格处理 `11`、`12`、`13`（以及 `111`、`112`、`113` 等 `n % 100` 落在 `11..=13` 的数）后缀统一为 `"th"`，其余按个位 `1→"st"`、`2→"nd"`、`3→"rd"`、其他→`"th"`。
- **任意进制范围约束（`formatRadixU64`）**：仅接受 `2 <= radix <= 36`，支持大小写字母切换（`0-9a-z` / `0-9A-Z`）；越界 `radix` 安全返回 `null`。
- **精确金融/指标定点数（`formatFixedPoint`）**：以缩放整数 `scaled_val` + 小数位数 `decimals` 格式化，无浮点二进制舍入误差，适合货币、百分比与性能延迟报表。

## 导出 API

| 函数签名 | 说明 |
| :--- | :--- |
| `formatGroupedU64(val, sep, out): ([]const u8)?` | 无符号 64 位整数千位分组格式化 |
| `formatGroupedI64(val, sep, out): ([]const u8)?` | 有符号 64 位整数千位分组格式化（安全处理 `i64::MIN`） |
| `ordinalSuffix(n): []const u8` | 返回英文序数词后缀（`"st"` / `"nd"` / `"rd"` / `"th"`） |
| `formatOrdinal(n, out): ([]const u8)?` | 输出带序数后缀的完整数字串（如 `"21st"`、`"11th"`） |
| `formatRadixU64(val, radix, uppercase, out): ([]const u8)?` | 2..36 进制格式化（越界进制或缓冲不足返回 `null`） |
| `formatFixedPoint(scaled_val, decimals, sep, out): ([]const u8)?` | 定点数小数与千位分组格式化 |

## 使用示例

```cp
import {
    formatGroupedU64,
    formatOrdinal,
    formatRadixU64,
    formatFixedPoint,
} from "cap:number_format";

fn demo() {
    let b1: [32]u8 = undefined;
    let b2: [16]u8 = undefined;
    let b3: [32]u8 = undefined;
    let b4: [32]u8 = undefined;
    const g = formatGroupedU64(1_234_567, ',', b1[0..]).?; // "1,234,567"
    const ord = formatOrdinal(112, b2[0..]).?;              // "112th"
    const b36 = formatRadixU64(255, 16, true, b3[0..]).?;   // "FF"
    const fp = formatFixedPoint(123456, 2, ',', b4[0..]).?; // "1,234.56"
    * = g; * = ord; * = b36; * = fp;
}
```
