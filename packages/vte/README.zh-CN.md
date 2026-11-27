[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# vte

Zero-allocation ANSI/VT100 terminal escape sequence state machine parser (Paul Williams model) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:vte` ([https://github.com/alacritty/vte](https://github.com/alacritty/vte))

| 元信息 | 值 |
| --- | --- |
| 包名 | `vte` |
| 版本 | `1.0.0` |
| 许可证 | `Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add vte
```

```cp
import {
    VteActionKind,
    VteState,
    VteEvent,
    VteParser,
    vtepZeroParams,
    vtepZeroOsc,
    vtepIsExecute,
    vtepAddDigit,
} from "vte";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `VteActionKind` | `Print, Execute, CsiDispatch, OscDispatch, EscDispatch` |
| `enum` | `VteState` | `Ground, Escape, CsiEntry, CsiParam, OscString, CsiIgnore, OscEsc` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `VteEvent` | `class` | 字段: `kind: VteActionKind, ch: u8, final_byte: u8, param_count: usize, params: [8]u16, intermediate: u8` |
| `VteParser` | `class` | 字段: `state: VteState, params: [8]u16, param_count: usize, cur_param: u16, has_param: bool, intermediate: u8, +2 项` · 方法: `new, reset, oscLen, oscByte, clearCsi, pushParam, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `vtepZeroParams` | `(): [8]u16` | 调用 `vtepZeroParams` 并返回 `[8]u16`。 |
| `vtepZeroOsc` | `(): [64]u8` | 调用 `vtepZeroOsc` 并返回 `[64]u8`。 |
| `vtepIsExecute` | `(b: u8): bool` | 调用 `vtepIsExecute` 并返回 `bool`。 |
| `vtepAddDigit` | `(cur: u16, digit: u8): u16` | 调用 `vtepAddDigit` 并返回 `u16`。 |
| `vtepParseOscCmd` | `(buf: &[]u8, len: usize): u16` | 调用 `vtepParseOscCmd` 并返回 `u16`。 |
| `vtepHasOscCmd` | `(buf: &[]u8, len: usize): bool` | 调用 `vtepHasOscCmd` 并返回 `bool`。 |
| `vtepMakeEvent` | `(kind: VteActionKind, ch: u8, final_byte: u8, param_count: usize, params: [8]u16, intermediate: u8): VteEvent` | 调用 `vtepMakeEvent` 并返回 `VteEvent`。 |

## 最小调用示例

```cp
import { vtepZeroParams } from "vte";

pub fn main() {
    let result = vtepZeroParams();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
