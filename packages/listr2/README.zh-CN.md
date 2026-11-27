[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# listr2

Zero-heap CLI task list state machine with pending/running/completed/skipped/failed lifecycle, retries, and ANSI rendering (npm listr2 equivalent, reuses picocolors)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:listr2` ([https://github.com/listr2/listr2](https://github.com/listr2/listr2))

| 元信息 | 值 |
| --- | --- |
| 包名 | `listr2` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add listr2
```

```cp
import { ListrOptions, TaskNode, ListrRunner, lstrPush } from "listr2";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ListrOptions` | `class` | 字段: `concurrent: bool, exit_on_error: bool, use_color: bool` · 方法: `defaults` |
| `TaskNode` | `class` | 字段: `title: string, state: u8, retries_left: u8, attempts: u8, skip_reason: string, err_msg: string` |
| `ListrRunner` | `class` | 字段: `opts: ListrOptions, tasks: [32]TaskNode, task_count: usize, halted: bool` · 方法: `new, addTask, startTask, completeTask, skipTask, failTask, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `lstrPush` | `(out: []u8, pos: usize, s: string): usize` | 调用 `lstrPush` 并返回 `usize`。 |

## 最小调用示例

```cp
import { lstrPush } from "listr2";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = lstrPush(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
