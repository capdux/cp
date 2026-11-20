# CP 标准库：定点多币种货币算术与外汇三角换算引擎 (`cap:fx` / `cap.fx`)

## 一、 架构总览

`cap:fx` 是 CP 语言官方内置的纯原生定点多币种货币算术与外汇三角换算引擎，采用 `i128` 最小货币单位（Minor Units）与 256 位无堆中间算术彻底消除 IEEE 754 浮点误差，专为跨境清结算、多币种账本、高并发外汇路由与零分币丢失分账场景设计。全模块零 `std.posix`、零 `@import("bun")`，支持 `import { ... } from "cap:fx"` 与 `import cap from "cap"; cap.fx.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/fx/money.cp` | `// Source: cp/lib/fx/money.cp (Fixed-Point Multi-Currency Money Arithmetic & ISO 4217 Registry)` | `FxError`、`RoundingMode`、`Currency`（ISO-4217 小数位注册表）、`Money`（定点解析/格式化、防混算、256 位舍入缩放、Hamilton 零分币丢失分账） |
| `lib/fx/rates.cp` | `// Source: cp/lib/fx/rates.cp (FX Rate Registry & 1-Hop Triangulation Engine)` | `RATE_SCALE_E9`、`FxRate`（1e-9 定点汇率与安全校验）、`FxBook`（直连、自动逆汇率、1 跳枢纽三角换算与跨 `minorUnits` 精确兑换） |
| `lib/fx/index.cp` | `// Source: cp/lib/fx/index.cp (Unified Exports for cap:fx)` | `cap:fx` 与 `cap.fx` 统一公开导出入口 |

---

## 二、 核心 API 速查

### 1. 错误集、舍入模式与 ISO-4217 货币注册表（`money.cp`）

- **`FxError`**：
  - `CurrencyMismatch`：跨币种加减拦截
  - `UnknownCurrency`：未知或非法 3 位大写币种码
  - `InvalidAmountFormat`：非法金额字符串或超出币种 `minorUnits` 精度
  - `Overflow`：算术或转换超出 `i128`/`u128` 范围
  - `DivisionByZero`：`mulScale` 分母为 0
  - `RateNotFound`：直连、逆向与 1 跳三角路径均未找到汇率
  - `InvalidRate`：汇率非有限正数（`<= 0`、`NaN`、`Inf`）
  - `InvalidAllocationWeights`：分账权重数组为空或总权重为 0
  - `OutOfMemory`：内存分配失败或自定义币种表满
- **`RoundingMode`**：`HalfUp`、`HalfEven`（银行家舍入）、`Floor`、`Ceil`、`Trunc`。
- **`Currency`**：
  - 内置常见 ISO-4217 货币：`USD`(2)、`EUR`(2)、`GBP`(2)、`CNY`(2)、`JPY`(0)、`KRW`(0)、`CHF`(2)、`HKD`(2)、`SGD`(2)、`AUD`(2)、`CAD`(2)、`NZD`(2)、`TWD`(2)、`INR`(2)、`KWD`(3)、`BHD`(3)、`OMR`(3)、`BTC`(8)、`ETH`(18)。
  - `Currency.register(code, minor_units)`：注册或更新自定义 3 位大写币种。
  - `Currency.minorUnits(code)`：查询内置或已注册自定义币种的小数位数。
  - `Currency.of(code)`：返回 `Currency { code, minor_units }`。

### 2. 定点货币值 `Money`（`money.cp`）

- **字段**：`amount_minor: i128`、`currency: [3]u8`。
- **构造与格式化**：
  - `Money.fromMinor(amount: i128, code: []const u8): Money ! FxError`
  - `Money.parse(decimal_str: []const u8, code: []const u8): Money ! FxError`（如 `"12.34", "USD"` -> `1234`）
  - `m.format(allocator): ([]u8) ! FxError`（返回如 `"12.34 USD"`、`"1000 JPY"`、`"0.00000001 BTC"`）
- **算术与分账**：
  - `m.add(other)` / `m.sub(other)`：同币种精确加减，异币种返回 `FxError.CurrencyMismatch`。
  - `m.mulScale(num: i128, den: i128, rounding: RoundingMode)`：256 位中间精度乘除与舍入。
  - `m.allocate(allocator, weights: []const u64): ([]Money) ! FxError`：基于 Hamilton 最大余数法按权重分摊最小货币单位，保证分摊总和严格守恒、零分币丢失，且零权重分片绝不获分余数。

### 3. 外汇汇率表与三角路由引擎（`rates.cp`）

- **`FxRate`**：`base: [3]u8`、`quote: [3]u8`、`rate_e9: u128`；支持 `FxRate.fromFloat(base, quote, rate)`、`FxRate.fromE9(base, quote, rate_e9)` 与 `toFloat()`。
- **`FxBook`**：
  - `FxBook.init(allocator)` / `book.deinit()`
  - `book.setRate(base, quote, rate: f64)` / `book.setRateE9(base, quote, rate_e9: u128)`
  - `book.getRate(base, quote): FxRate ! FxError`：依次尝试直连 `(base, quote)`、自动逆向 `(quote, base)` 与 1 跳公共枢纽币种（优先 `USD`/`EUR`，再遍历已注册币种）三角推导。
  - `book.convert(money, target_code, rounding): Money ! FxError`：使用约简后的精确有理汇率与 `10^(dst_mu - src_mu)` 标度因子执行 256 位无损跨 `minorUnits` 兑换（如 `USD(2)` <-> `JPY(0)` <-> `BTC(8)`）。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { Currency, Money, FxBook, RoundingMode } from "cap:fx";

pub fn demoFx()! {
    const alloc = std.heap.page_allocator;

    const price = Money.parse("12.34", "USD")?;
    const formatted = price.format(alloc)?;
    defer alloc.free(formatted); // "12.34 USD"

    const weights: [3]u64 = [1, 1, 1];
    const parts = (Money.parse("100.00", "USD")?).allocate(alloc, &weights)?;
    defer alloc.free(parts); // [33.34 USD, 33.33 USD, 33.33 USD]

    let book = cap.fx.FxBook.init(alloc);
    defer book.deinit();
    book.setRate("USD", "CNY", 7.20)?;
    book.setRate("USD", "JPY", 150.0)?;

    const cny = Money.parse("720.00", "CNY")?;
    const jpy = book.convert(cny, "JPY", RoundingMode.HalfEven)?; // 15000 JPY
    * = jpy;
}
```
