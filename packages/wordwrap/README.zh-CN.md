[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# wordwrap

Zero-heap column-width text word wrapper with indentation, soft/hard modes, and newline preservation in pure CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `wordwrap` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add wordwrap
```

```cp
import {
    WwrpEmitRes,
    wwrpIsInlineWs,
    wwrpPushByte,
    wwrpPushSpaces,
    wwrpPushSlice,
    wwrpEmitWord,
    wrapText,
} from "wordwrap";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `WwrpEmitRes` | `class` | 字段: `pos: usize, line_len: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `wwrpIsInlineWs` | `(c: u8): bool` | 调用 `wwrpIsInlineWs` 并返回 `bool`。 |
| `wwrpPushByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `wwrpPushByte` 并返回 `usize`。 |
| `wwrpPushSpaces` | `(out: []u8, pos: usize, count: usize): usize` | 调用 `wwrpPushSpaces` 并返回 `usize`。 |
| `wwrpPushSlice` | `(out: []u8, pos: usize, s: string): usize` | 调用 `wwrpPushSlice` 并返回 `usize`。 |
| `wwrpEmitWord` | `(word: string, start_col: usize, max_w: usize, hard_mode: bool, out: []u8, start_pos: usize, cur_line_len: usize): WwrpEmitRes` | 调用 `wwrpEmitWord` 并返回 `WwrpEmitRes`。 |
| `wrapText` | `(input: string, start_col: usize, stop_col: usize, hard_mode: bool, out: []u8): usize` | 调用 `wrapText` 并返回 `usize`。 |

## 最小调用示例

```cp
import { wwrpPushSlice } from "wordwrap";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = wwrpPushSlice(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
