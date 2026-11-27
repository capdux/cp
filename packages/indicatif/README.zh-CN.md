[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# indicatif

Zero-heap terminal progress bar, throughput, and ETA renderer composing pretty-bytes and pretty-ms in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:indicatif` ([https://github.com/console-rs/indicatif](https://github.com/console-rs/indicatif))

| 元信息 | 值 |
| --- | --- |
| 包名 | `indicatif` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add indicatif
```

```cp
import {
    ProgressState,
    indWriteByte,
    indWriteStr,
    indWriteU64,
    percent,
    etaMs,
    bytesPerSec,
    renderBar,
} from "indicatif";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ProgressState` | `class` | 字段: `pos: u64, total: u64, elapsed_ms: u64, width: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `indWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `indWriteByte` 并返回 `usize`。 |
| `indWriteStr` | `(out: []u8, pos: usize, s: &[]u8): usize` | 调用 `indWriteStr` 并返回 `usize`。 |
| `indWriteU64` | `(out: []u8, pos: usize, val: u64): usize` | 调用 `indWriteU64` 并返回 `usize`。 |
| `percent` | `(s: ProgressState): f64` | 调用 `percent` 并返回 `f64`。 |
| `etaMs` | `(s: ProgressState): u64` | 调用 `etaMs` 并返回 `u64`。 |
| `bytesPerSec` | `(s: ProgressState): f64` | 调用 `bytesPerSec` 并返回 `f64`。 |
| `renderBar` | `(s: ProgressState, fill_ch: u8, head_ch: u8, empty_ch: u8, out: []u8): usize` | 调用 `renderBar` 并返回 `usize`。 |
| `renderLine` | `(s: ProgressState, prefix: string, out: []u8): usize` | 调用 `renderLine` 并返回 `usize`。 |

## 最小调用示例

```cp
import { renderLine } from "indicatif";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = renderLine(/* s: ProgressState */, "example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
