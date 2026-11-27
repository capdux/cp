[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# fastq

Zero-allocation fast in-memory fixed-concurrency work queue with push, unshift, pause, resume, kill, and drain (clean-room MIT implementation of npm fastq, #253 / 192M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fastq` ([https://github.com/mcollina/fastq](https://github.com/mcollina/fastq)) | 许可：`ISC`

| 元信息 | 值 |
| --- | --- |
| 包名 | `fastq` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add fastq
```

```cp
import { FastQueue } from "fastq";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FastQueue` | `class` | 字段: `buf: [64]u64, head: usize, tail: usize, queued: usize, active: usize, concurrency: usize, +1 项` · 方法: `new, length, running, idle, isPaused, setConcurrency, +7 项` |

## 最小调用示例

```cp
import { FastQueue } from "fastq";

pub fn main() {
    // Refer to src/index.d.cp for FastQueue fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
