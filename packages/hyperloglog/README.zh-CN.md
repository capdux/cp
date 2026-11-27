[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# hyperloglog

Zero-allocation 256-byte HyperLogLog cardinality estimator with LinearCounting small-range correction and distributed merge (Rust hyperloglog / Redis PFADD/PFCOUNT/PFMERGE parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:hyperloglog` ([https://github.com/jedisct1/rust-hyperloglog](https://github.com/jedisct1/rust-hyperloglog))

| 元信息 | 值 |
| --- | --- |
| 包名 | `hyperloglog` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add hyperloglog
```

```cp
import { HyperLogLog256, hashBytes64, hashU64, clz56 } from "hyperloglog";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `HyperLogLog256` | `class` | 字段: `regs: [HLL_M]u8` · 方法: `init, clear, registerCount, insertHash, add, addU64, +3 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hashBytes64` | `(data: string): u64` | 执行 `hashBytes64` 条件判定，返回 `bool`。 |
| `hashU64` | `(val: u64): u64` | 执行 `hashU64` 条件判定，返回 `bool`。 |
| `clz56` | `(w: u64): u8` | 调用 `clz56` 并返回 `u8`。 |

## 最小调用示例

```cp
import { hashBytes64 } from "hyperloglog";

pub fn main() {
    let result = hashBytes64("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
