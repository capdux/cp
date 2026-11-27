[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bimap

Zero-heap bidirectional 1-to-1 map (left <-> right) in pure CP, modeled after Rust bimap

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bimap` ([https://github.com/billyrieger/bimap-rs](https://github.com/billyrieger/bimap-rs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `bimap` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bimap
```

```cp
import { BiPair, BiMap } from "bimap";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BiPair` | `class` | 字段: `left: u64, right: u64` |
| `BiMap` | `class` | 字段: `pairs: [128]BiPair, count: usize` · 方法: `init, len, isEmpty, capacity, findLeftIndex, findRightIndex, +9 项` |

## 最小调用示例

```cp
import { BiPair } from "bimap";

pub fn main() {
    // Refer to src/index.d.cp for BiPair fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
