[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# minipass

Zero-allocation fixed-capacity ring-buffer byte stream with synchronous backpressure and piping for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:minipass` ([https://github.com/isaacs/minipass](https://github.com/isaacs/minipass))

| 元信息 | 值 |
| --- | --- |
| 包名 | `minipass` |
| 版本 | `1.0.0` |
| 许可证 | `ISC` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add minipass
```

```cp
import { MinipassStream, mpassMin } from "minipass";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MinipassStream` | `class` | 字段: `buf: [4096]u8, head: usize, len: usize, flowing: bool, ended: bool, destroyed: bool` · 方法: `new, bufferedBytes, freeSpace, isPaused, isEnded, isDrained, +9 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mpassMin` | `(a: usize, b: usize): usize` | 调用 `mpassMin` 并返回 `usize`。 |

## 最小调用示例

```cp
import { mpassMin } from "minipass";

pub fn main() {
    let result = mpassMin(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
