[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# fixed

Zero-allocation signed 64-bit Q32.32 deterministic fixed-point arithmetic engine with overflow-free 128-bit split mul/div, Newton sqrt, lerp, and clamp (Rust fixed parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:fixed` ([https://gitlab.com/tspiteri/fixed](https://gitlab.com/tspiteri/fixed))（原库名 `fixed` → CP 包名 `fixed`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `fixed` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add fixed
```

```cp
import { Fixed32, absU64, divShift32U64 } from "fixed";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Fixed32` | `class` | 字段: `raw: i64` · 方法: `fromInt, fromRaw, fromRatio, fromF64, rawValue, toF64, +14 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `absU64` | `(v: i64): u64` | 调用 `absU64` 并返回 `u64`。 |
| `divShift32U64` | `(a: u64, b: u64): u64` | 调用 `divShift32U64` 并返回 `u64`。 |

## 最小调用示例

```cp
import { absU64 } from "fixed";

pub fn main() {
    let result = absU64(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
