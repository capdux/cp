# CP 标准库：高精度 128 位十进制金融算术与货币引擎 (`cap:decimal` / `cap.decimal`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

## 一、 架构总览

`cap:decimal` 是 CP 语言官方内置的纯原生 128 位定点十进制算术与 ISO 4217 货币结算标准库，彻底消除 IEEE 754 二进制浮点精度漂移（`0.1 + 0.2 == 0.3` 精确成立），专为金融清结算、AI Token 微额计费、发票税率与会计分账系统打造。全模块零 C 依赖、零 `std.posix`、零 `@import("bun")`，支持 `import { ... } from "cap:math"` 与 `import cap from "cap"; cap.decimal.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/decimal/decimal.cp` | `// Source: cp/lib/decimal/decimal.cp (128-bit Fixed-Point Decimal Arithmetic & Rounding)` | `MAX_SCALE`、`DecimalError`、`RoundingMode`、`Decimal`（128 位定点数 + 384 位无堆中间算术与 5 种舍入模式） |
| `lib/decimal/money.cp` | `// Source: cp/lib/decimal/money.cp (ISO 4217 Currency-Safe Money & Penny-Exact Allocation)` | `MoneyError`、`Money`（ISO 4217 币种隔离防混算 + Hamilton 最大余数零分钱丢失分账引擎） |
| `lib/decimal/index.cp` | `// Source: cp/lib/decimal/index.cp (Unified Exports for cap:decimal)` | `cap:decimal` 与 `cap.decimal` 统一公开导出入口 |

---

## 二、 核心 API 速查

### 1. 高精度 128 位定点十进制数（`decimal.cp`）

- **`MAX_SCALE: u8 = 28`**：支持 `0..=28` 位小数标度。
- **`DecimalError`**：`InvalidFormat`、`Overflow`、`DivisionByZero`、`InvalidScale`、`OutOfMemory`。
- **`RoundingMode`**：
  - `HalfUp`：四舍五入（半值远离零：`2.5 -> 3`，`-2.5 -> -3`）
  - `HalfEven`：银行家舍入（半值向最近偶数舍入：`2.5 -> 2`，`3.5 -> 4`）
  - `Floor`：向负无穷方向舍入
  - `Ceil`：向正无穷方向舍入
  - `Trunc`：向零截断
- **`Decimal`**（值类型：`mantissa: i128`, `scale: u8`）：
  - **构造与解析**：`zero()`、`one()`、`fromInt(v: i64)`、`fromI128(v: i128)`、`fromScaled(mantissa: i128, scale: u8): Decimal ! DecimalError`、`parse(input: []const u8): Decimal ! DecimalError`（支持正负号、小数点与科学计数法如 `"1.5e-4"`）。
  - **核心算术**：`add(other)`、`sub(other)`、`mul(other)`、`div(other, target_scale, mode)`（防除零 `DivisionByZero` 与溢出 `Overflow`）、`neg()`、`abs()`。
  - **比较与标度**：`cmp(other): i8`、`eql(other): bool`（规范化数学相等：`1.20 == 1.2`）、`normalize(): Decimal`（去除尾随零）、`rescale(target_scale, mode)`、`round(target_scale, mode)`、`isZero()` / `isNegative()` / `isPositive()`。
  - **格式化**：`formatBuf(buf: []u8): ([]const u8) ! DecimalError`、`format(alloc): ([]u8) ! DecimalError`。

### 2. ISO 4217 币种安全货币与零分钱丢失分账（`money.cp`）

- **`MoneyError`**：`CurrencyMismatch`、`InvalidCurrency`、`InvalidShares`。
- **`Money`**（`currency: [3]u8`、`amount: Decimal`、`minor_units: u8`）：
  - `init(currency: []const u8, amount: Decimal, minor_units: u8)`：严格校验 3 位大写 ISO 4217 币种码（如 `"USD"`、`"CNY"`、`"EUR"`、`"JPY"`）与 `minor_units <= 28`。
  - `add(other)` / `sub(other)`：同币种精确加减，异币种或最小单位不匹配严格返回 `MoneyError.CurrencyMismatch`。
  - `mulFactor(factor: Decimal, mode: RoundingMode)`：按费率/汇率相乘并按 `minor_units` 与 `mode` 舍入。
  - `toMinorUnits(): i128 ! DecimalError` / `eql(other): bool` / `format(alloc): ([]u8) ! DecimalError`。
  - `allocateEven(alloc, n: usize)`：均匀分账并逐分分配余数（如 `$100.00 / 3 -> [$33.34, $33.33, $33.33]`，总和 100% 严格等于原金额）。
  - `allocateRatios(alloc, ratios: []const u64)`：基于 Hamilton 最大余数法的权重比例分配，确保总和 100% 零分钱丢失且零权重分片绝不获分余数。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { Decimal, Money, RoundingMode } from "cap:math";

pub fn demoSettlement()! {
    const alloc = std.heap.page_allocator;

    const a = Decimal.parse("0.1")?;
    const b = Decimal.parse("0.2")?;
    const sum = a.add(b)?; // 精确等于 0.3

    const price = Decimal.parse("1.5e-6")?; // AI Token 单价 $0.0000015 / token
    const tokens = Decimal.fromInt(2000000);
    const total = price.mul(tokens)?;      // 精确等于 $3.00

    const bill = Money.init("USD", Decimal.parse("100.00")?, 2)?;
    const shares = bill.allocateEven(alloc, 3)?;
    defer alloc.free(shares);
    // shares = [USD 33.34, USD 33.33, USD 33.33]

    const fee = cap.decimal.Decimal.parse("2.5")?;
    * = sum.eql(Decimal.parse("0.3")?);
    * = total;
    * = fee.round(0, .HalfEven)?; // 2
}
```
