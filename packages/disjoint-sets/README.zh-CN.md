[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# disjoint-sets

Zero-heap Disjoint Set Union (Union-Find) with path halving and union by size/rank for pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:disjoint-sets` ([https://github.com/tov/disjoint-sets-rs](https://github.com/tov/disjoint-sets-rs))（原库名 `disjoint-sets` → CP 包名 `disjoint-sets`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `disjoint-sets` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add disjoint-sets
```

```cp
import { DisjointSet } from "disjoint-sets";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DisjointSet` | `class` | 字段: `parent: [256]u16, sizes: [256]u16, count: usize, components: usize` · 方法: `init, len, componentCount, find, findReadOnly, connected, +3 项` |

## 最小调用示例

```cp
import { DisjointSet } from "disjoint-sets";

pub fn main() {
    // Refer to src/index.d.cp for DisjointSet fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
