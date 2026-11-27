[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# deranged

适用于 CP 语言的零堆分配有界范围整数库，对齐 Rust [`deranged`](https://github.com/jhpratt/deranged) crate（MIT 协议，CP v1.0.0）。

提供覆盖全部 8 种定宽整数的值类型封装（`RangedI8`、`RangedI16`、`RangedI32`、`RangedI64`、`RangedU8`、`RangedU16`、`RangedU32`、`RangedU64`），支持边界检查运算、饱和运算、区间环绕运算（`RangedI32`/`RangedI64`）以及范围扩缩操作，非常适合日期时间、端口号、百分比和协议字段校验。

## 安装与导入

```bash
cap add deranged
```

```cp
import { RangedU8, RangedI32, RangedI64, RangedU16 } from "deranged";
```

## 核心 API 参考

### 支持的类型

全部 8 种定宽整数共享统一的值布局 `{ val: T, min_v: T, max_v: T, ok: bool }`：

- 有符号：`RangedI8`（`i8`）、`RangedI16`（`i16`）、`RangedI32`（`i32`）、`RangedI64`（`i64`）
- 无符号：`RangedU8`（`u8`）、`RangedU16`（`u16`）、`RangedU32`（`u32`）、`RangedU64`（`u64`）

### 通用方法（全部 8 种类型均支持）

| 方法 | 签名（`T` 为底层整数类型） | 说明 |
| :--- | :--- | :--- |
| `new(val, min_v, max_v)` | `(T, T, T): RangedT` | 构造有界整数；若 `min_v > max_v` 或 `val` 越界则置 `ok = false`。 |
| `newSaturating(val, min_v, max_v)` | `(T, T, T): RangedT` | 将 `val` 钳位到 `[min_v, max_v]`（仅在 `min_v > max_v` 时 `ok = false`）。 |
| `get()` | `(): T` | 获取当前存储的整数值。 |
| `min()` / `max()` | `(): T` | 获取闭区间下界 / 上界。 |
| `isValid()` | `(): bool` | 判断实例是否有效（`ok == true` 且在边界内）。 |
| `inBounds(v)` | `(T): bool` | 检查 `v` 是否落在 `[min_v, max_v]` 内。 |
| `checkedAdd(delta)` | `(T): RangedT` | 带溢出与区间检查的加法；越界时返回 `ok = false`。 |
| `checkedSub(delta)` | `(T): RangedT` | 带下溢与区间检查的减法；越界时返回 `ok = false`。 |
| `saturatingAdd(delta)` | `(T): RangedT` | 饱和加法，结果钳位到 `[min_v, max_v]`。 |
| `saturatingSub(delta)` | `(T): RangedT` | 饱和减法，结果钳位到 `[min_v, max_v]`。 |
| `expand(new_min, new_max)` | `(T, T): RangedT` | 扩大区间边界（需满足 `new_min <= min_v && new_max >= max_v`），否则 `ok = false`。 |
| `narrow(new_min, new_max)` | `(T, T): RangedT` | 收窄区间边界（需当前值仍落在 `[new_min, new_max]` 内），否则 `ok = false`。 |

### 区间环绕算术方法（仅 `RangedI32` 与 `RangedI64`）

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `wrappingAdd(delta)` | `(T): RangedT` | 在 `[min_v, max_v]` 闭区间内执行模环绕加法（如时钟小时/分钟）。 |
| `wrappingSub(delta)` | `(T): RangedT` | 在 `[min_v, max_v]` 闭区间内执行模环绕减法。 |

## 代码示例

### 1. 月份与百分比边界校验（`RangedU8`）

```cp
import { RangedU8 } from "deranged";

fn validateMonth() {
    let month = RangedU8.new(12, 1, 12);
    if (month.isValid()) {
        let overflow = month.checkedAdd(1);   // overflow.isValid() == false
        let clamped = month.saturatingAdd(5); // clamped.get() == 12
    }

    let pct = RangedU8.newSaturating(150, 0, 100);
    // pct.isValid() == true, pct.get() == 100
}
```

### 2. 时钟模环绕运算（`RangedI32`）

```cp
import { RangedI32 } from "deranged";

fn clockExample() {
    let hour = RangedI32.new(23, 0, 23);
    let next = hour.wrappingAdd(3); // next.get() == 2
    let prev = next.wrappingSub(5); // prev.get() == 21
}
```

### 3. 范围扩缩（`RangedI64`）

```cp
import { RangedI64 } from "deranged";

fn rangeTransform() {
    let v = RangedI64.new(50, 0, 100);
    let wide = v.expand(-100, 200);      // wide.isValid() == true
    let tight = wide.narrow(10, 60);     // tight.isValid() == true, tight.get() == 50
    let invalid = tight.narrow(60, 100); // invalid.isValid() == false
}
```

## 与源包差异与安全亮点

- **运行时动态边界 + 零堆分配**：Rust 原版使用常量泛型编码边界（`RangedU8<MIN, MAX>`）；CP 版将 `min_v`、`max_v` 与 `ok` 直接存放在紧凑值结构体中，支持动态边界且全程栈分配。
- **永不 Panic**：所有构造与算术操作通过 `ok: bool`（`isValid()`）显式传播合法性，不会因溢出或越界触发运行时崩溃。
- **防原生整数溢出**：内部在做区间比较前先防护底层整数溢出，保证无符号与有符号极值边界计算安全。
