[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查手册](README_AGENT.md)

# deranged

面向 CP 的零堆内存有界范围整数库，提供全部 8 种有符号/无符号位宽（`RangedU8`/`U16`/`U32`/`U64`、`RangedI8`/`I16`/`I32`/`I64`），支持检查运算、饱和运算、环绕运算及区间扩缩。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| **上游源平台** | [crates.io/crates/deranged](https://crates.io/crates/deranged) |
| **上游原包名** | `deranged`（CP 版本 `1.0.0`） |
| **上游仓库** | https://github.com/jhpratt/deranged |
| **许可证** | MIT |

从 Rust 迁移到 CP 保持原包名不变：

```rust
// Rust
use deranged::{RangedU8, RangedI32};
```

```cp
// CP —— 原包名直接导入，零堆分配
import { RangedU8, RangedI32, RangedI64 } from "deranged";
```

## 安装

```bash
cap add deranged
```

## 实战示例

### 1. 构造、合法性校验与饱和截断

```cp
import { RangedU8, RangedU16, RangedI32 } from "deranged";

fn demoBounds() {
    // 合法月份 [1, 12]
    let month = RangedU8.new(5, 1, 12);
    if (month.isValid()) {
        let v = month.get();  // 5
        let lo = month.min(); // 1
        let hi = month.max(); // 12
    }

    // 越界构造不会 panic，而是标记 ok = false
    let invalid = RangedU8.new(13, 1, 12);
    // invalid.isValid() == false

    // 饱和构造自动将值钳位到 [min, max]
    let clamped = RangedU16.newSaturating(7000, 1024, 6553);
    // clamped.isValid() == true, clamped.get() == 6553
}
```

### 2. 检查运算、饱和运算与环绕运算

```cp
import { RangedI32, RangedI64, RangedU32 } from "deranged";

fn demoArithmetic() {
    let hour = RangedI32.new(22, 0, 23);

    // 检查加法：超出 [0, 23] 返回 invalid 实例
    let overflow = hour.checkedAdd(5);
    // overflow.isValid() == false

    // 饱和加法：钳位至上界 23
    let sat = hour.saturatingAdd(5);
    // sat.get() == 23

    // 环绕加法（RangedI32 / RangedI64 支持）：按区间长度取模回绕
    let wrapped = hour.wrappingAdd(5);
    // wrapped.get() == 3

    // 无符号范围减法防下溢保护
    let pct = RangedU32.new(10, 0, 100);
    let safe_sub = pct.saturatingSub(50); // 0
    let bad_sub = pct.checkedSub(50);     // isValid() == false
}
```

### 3. 区间扩展与收窄（`expand` / `narrow`）

```cp
import { RangedI8, RangedU64 } from "deranged";

fn demoRangeTransform() {
    let temp = RangedI8.new(25, 0, 50);

    // 扩展边界到更宽区间（新区间须覆盖原 [min, max]）
    let wider = temp.expand(-40, 100);
    // wider.isValid() == true, wider.get() == 25

    // 收窄边界到更紧区间（当前值超出新区间时标记无效）
    let tighter = temp.narrow(20, 30);
    // tighter.isValid() == true

    let fail_narrow = temp.narrow(30, 40);
    // fail_narrow.isValid() == false
}
```

## 性能与安全亮点

- **零堆纯栈值类型**：所有 `Ranged*` 类型均为紧凑值结构体（`val`、`min_v`、`max_v`、`ok`），零堆分配、零 GC 压力。
- **零 Panic 溢出防护**：检查运算与饱和运算同时防护区间越界与原生整数上下溢。
- **全位宽覆盖**：完整覆盖 8 种整型（`U8`/`U16`/`U32`/`U64`/`I8`/`I16`/`I32`/`I64`），适用于日期时间库、协议字段与配置参数边界约束。

## API 参考

完整类型定义见 [`src/index.d.cp`](src/index.d.cp)，AI 智能体极简速查见 [`README_AGENT.md`](README_AGENT.md)。
