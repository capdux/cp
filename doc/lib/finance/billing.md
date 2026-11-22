# CP 标准库：定量计费、阶梯定价、周期折算与发票引擎 (`cap:billing` / `cap.billing`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:finance`，导入请使用 `import { ... } from "cap:finance"`。

## 一、 架构总览

`cap:billing` 是 CP 语言官方内置的纯原生、零 C 依赖工业级计费与发票生成标准库，专为 AI API Token 计量、SaaS 订阅升降级折算与云资源账单结算打造。全模块使用最小整数货币单位（`cents` / `micros`，`i64` / `i128` 中间运算）保证零分钱浮点精度误差，严禁 `std.posix` 与 `@import("bun")`，支持 `import { ... } from "cap:finance"` 与 `import cap from "cap"; cap.billing.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/billing/billing.cp` | `// Source: cp/lib/billing/billing.cp (Quantitative Billing, Tiered Pricing, Proration & Invoicing)` | 错误集、定价模型、阶梯计价、周期折算、折扣与税费核算、结构化 `Invoice` 与 RFC 8259 JSON 序列化 |
| `lib/billing/index.cp` | `// Source: cp/lib/billing/index.cp (Unified Exports for cap:billing)` | `cap:billing` 与 `cap.billing` 统一公开导出入口 |

---

## 二、 核心 API 速查

### 1. 常量、错误集与枚举

- **常量**：
  - `BPS_DENOMINATOR: i128 = 10000`（基点分母，10,000 bps = 100.00%）
  - `MICROS_PER_CENT: i128 = 10000`（1 美分 = 10,000 micros，1 美元 = 1,000,000 micros）
- **`BillingError`**：`InvalidTiers`、`InvalidPeriod`、`InvalidDiscount`、`NegativeQuantity`、`InvalidTaxRate`、`OutOfMemory`
- **`TierMode`**：
  - `Graduated`：累进阶梯计价，每档仅对落入该区间的用量计费并叠加档位固定费
  - `Volume`：批量统一定档，全部用量统一按命中总量的档位单价与固定费计费
- **`CouponKind`**：
  - `PercentBps`：基点百分比折扣（`0..=10000`，如 `1500` = 15.00%）
  - `FixedAmount`：固定分（cents）减免，自动封顶至小计金额，绝不扣出负数应付额

### 2. 核心数据结构

- **`PriceTier`**：`up_to_units: u64?`（`null` 表示无限封顶末档）、`unit_price_micros: i64`、`flat_fee_micros: i64`
- **`Coupon`**：`code: []const u8`、`kind: CouponKind`、`value: i64`
- **`InvoiceLineItem`**：`description: []const u8`、`quantity: u64`、`amount_cents: i64`
- **`Invoice`**：
  - 字段：`customer_id`、`currency`、`lines: []InvoiceLineItem`、`subtotal_cents`、`discount_cents`、`tax_cents`、`total_cents`、`coupon_code: ([]const u8)?`、`tax_bps: u32`
  - 方法：`deinit(*_)` 释放堆分配的 `lines`；`toJson(alloc: std.mem.Allocator): ([]u8) ! BillingError` 导出标准 RFC 8259 JSON（带完整字符串转义）

### 3. `BillingCalculator` 核心方法

- **`priceMeteredUsage(units: u64, tiers: []const PriceTier, mode: TierMode): i64 ! BillingError`**
  - 校验 `tiers` 非空、单调严格递增且末档必须为 `up_to_units == null`（否则返回 `InvalidTiers`），并精确计算微单位（micros）总价。
- **`microsToCents(micros: i64): i64`**
  - 使用对称四舍五入（HalfUp）将微单位转换为分（cents）。
- **`calculateProration(old_plan_cents: i64, new_plan_cents: i64, period_start_ms: i64, period_end_ms: i64, switch_ms: i64): i64 ! BillingError`**
  - 校验 `period_start_ms < period_end_ms` 且 `period_start_ms <= switch_ms <= period_end_ms`（否则返回 `InvalidPeriod`）。采用对称整数四舍五入，升级返回正数补差，降级返回负数退费。
- **`buildInvoice(alloc: std.mem.Allocator, customer_id: []const u8, currency: []const u8, items: []const InvoiceLineItem, coupon: Coupon?, tax_bps: u32): Invoice ! BillingError`**
  - 汇总行项目、校验并应用优惠券（确保折后金额不低于 0）、按 `tax_bps <= 10000` 计算税费并生成 `Invoice`。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { BillingCalculator, PriceTier, TierMode, Coupon, InvoiceLineItem } from "cap:finance";

pub fn demoBilling(alloc: std.mem.Allocator)! {
    const tiers: []const PriceTier = [
        PriceTier{ up_to_units: 1000, unit_price_micros: 20, flat_fee_micros: 0 },
        PriceTier{ up_to_units: null, unit_price_micros: 10, flat_fee_micros: 500 },
    ];
    const micros = BillingCalculator.priceMeteredUsage(2500, tiers, .Graduated)?;
    const usage_cents = BillingCalculator.microsToCents(micros);

    const items: []const InvoiceLineItem = [
        InvoiceLineItem{ description: "Pro Plan", quantity: 1, amount_cents: 2900 },
        InvoiceLineItem{ description: "AI Tokens", quantity: 2500, amount_cents: usage_cents },
    ];
    const coupon = Coupon{ code: "LAUNCH10", kind: .PercentBps, value: 1000 };
    let inv = BillingCalculator.buildInvoice(alloc, "cust_001", "USD", items, coupon, 800)?;
    defer inv.deinit();

    const json_bytes = inv.toJson(alloc)?;
    defer alloc.free(json_bytes);

    * = cap.billing.BillingCalculator.calculateProration(2900, 9900, 0, 1000, 500)?;
}
```
