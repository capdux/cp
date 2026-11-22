# `cap:decimal_fp` — 工业级金融定点数精确算术引擎

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

## 1. 概述

`cap:decimal_fp`（同时支持 `import cap from "cap"` 下的 `cap.decimal_fp` 动态挂载）是纯 CP 实现的零浮点精度漂移金融定点数标准库。底层采用 `i128` 尾数（`mantissa`）与 `u8` 十进制标度（`scale`，最大 `MAX_SCALE = 18`），配合 `u256`/`i256` 中间态精确对齐与舍入，彻底消除二进制浮点误差（如 `"0.1" + "0.2" == "0.3"`），适用于账务、结算、汇率、费率与高精度金融计算场景。

## 2. 核心类型

- `MAX_SCALE: u8 = 18`：支持的最大十进制小数位数。
- `DecimalError`：`InvalidFormat` | `ScaleOutOfRange` | `Overflow` | `DivisionByZero`。
- `RoundingMode`：
  - `HalfUp`：四舍五入（绝对值一半向远离零方向舍入）
  - `HalfEven`：银行家舍入（一半向最近偶数舍入）
  - `Floor`：向负无穷方向舍入
  - `Ceil`：向正无穷方向舍入
- `Decimal`：`struct { mantissa: i128, scale: u8 }`

## 3. API 速查

- 构造与解析：
  - `Decimal.fromInt(v: i64): Decimal`
  - `Decimal.fromParts(mantissa: i128, scale: u8): DecimalError!Decimal`
  - `Decimal.parse(s: []const u8): DecimalError!Decimal`
- 格式化：
  - `d.formatAlloc(alloc: std.mem.Allocator): ![]u8`
- 精确算术与舍入：
  - `d.add(other: Decimal): DecimalError!Decimal`
  - `d.sub(other: Decimal): DecimalError!Decimal`
  - `d.mul(other: Decimal): DecimalError!Decimal`
  - `d.divRound(other: Decimal, target_scale: u8, mode: RoundingMode): DecimalError!Decimal`
  - `d.rescale(target_scale: u8, mode: RoundingMode): DecimalError!Decimal`
- 比较与符号变换：
  - `d.compare(other: Decimal): i8`（返回 `-1`、`0`、`1`）
  - `d.equals(other: Decimal): bool`
  - `d.neg(): Decimal`
  - `d.abs(): Decimal`

## 4. 示例

```cp
import { Decimal, RoundingMode } from "cap:math";

const a = Decimal.parse("0.1")?;
const b = Decimal.parse("0.2")?;
const c = a.add(b)?;
const expected = Decimal.parse("0.3")?;
// c.equals(expected) == true

const price = Decimal.parse("10.00")?;
const qty = Decimal.parse("3")?;
const unit = price.divRound(qty, 2, RoundingMode.HalfEven)?; // 3.33
```
