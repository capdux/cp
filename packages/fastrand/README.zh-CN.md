[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# fastrand

Zero-allocation WyRand 64-bit PRNG with unbiased range sampling, 53-bit unit floats, Fisher-Yates shuffle, and alphanumeric generation for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:fastrand` ([https://github.com/smol-rs/fastrand](https://github.com/smol-rs/fastrand))

| 元信息 | 值 |
| --- | --- |
| 包名 | `fastrand` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add fastrand
```

```cp
import {
    Rng,
    addWrap64,
    mulWide64Xor,
    i64ToOrderedU64,
    orderedU64ToI64,
} from "fastrand";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Rng` | `class` | 字段: `state: u64` · 方法: `withSeed, nextU64, nextU32, nextBool, fork, u64Below, +8 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `addWrap64` | `(a: u64, b: u64): u64` | 写入或更新状态数据。 |
| `mulWide64Xor` | `(a: u64, b: u64): u64` | 调用 `mulWide64Xor` 并返回 `u64`。 |
| `i64ToOrderedU64` | `(v: i64): u64` | 调用 `i64ToOrderedU64` 并返回 `u64`。 |
| `orderedU64ToI64` | `(u: u64): i64` | 调用 `orderedU64ToI64` 并返回 `i64`。 |

## 最小调用示例

```cp
import { addWrap64 } from "fastrand";

pub fn main() {
    let result = addWrap64(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
