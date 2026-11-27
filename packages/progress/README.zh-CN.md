[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# progress

Zero-heap terminal progress bar renderer supporting :bar, :percent, :current, :total, :eta, and :rate tokens

| 元信息 | 值 |
| --- | --- |
| 包名 | `progress` |
| 版本 | `2.0.3` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add progress
```

```cp
import { ProgressBar, newProgressBar, prgWriteBar, renderProgress } from "progress";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ProgressBar` | `class` | 字段: `total: u64, current: u64, width: usize, complete_char: u8, incomplete_char: u8, head_char: u8, +1 项` · 方法: `ratio, percent, isComplete, tick, etaSeconds` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `newProgressBar` | `(total: u64, width: usize): ProgressBar` | 创建或初始化实例，返回 `ProgressBar`。 |
| `prgWriteBar` | `(bar: ProgressBar, out: []u8, pos: usize): usize` | 调用 `prgWriteBar` 并返回 `usize`。 |
| `renderProgress` | `(bar: ProgressBar, template: string, out: []u8): usize` | 调用 `renderProgress` 并返回 `usize`。 |

## 最小调用示例

```cp
import { renderProgress } from "progress";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = renderProgress(/* bar: ProgressBar */, "example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
