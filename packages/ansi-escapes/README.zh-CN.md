[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ansi-escapes

Zero-allocation ANSI/VT100 terminal cursor control, line/screen erasing, OSC-8 hyperlinks, and window title escape sequence engine (ansi-escapes parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ansi-escapes` ([https://github.com/sindresorhus/ansi-escapes](https://github.com/sindresorhus/ansi-escapes))

| 元信息 | 值 |
| --- | --- |
| 包名 | `ansi-escapes` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ansi-escapes
```

```cp
import {
    pushByte,
    pushStr,
    pushUsize,
    cursorTo,
    cursorToXY,
    cursorMove,
    cursorUp,
    cursorDown,
} from "ansi-escapes";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pushByte` | `(out: []u8, pos: usize, b: u8): usize` | 写入或更新状态数据。 |
| `pushStr` | `(out: []u8, pos: usize, s: string): usize` | 写入或更新状态数据。 |
| `pushUsize` | `(out: []u8, pos: usize, val: usize): usize` | 写入或更新状态数据。 |
| `cursorTo` | `(x: usize, out: []u8): string` | 调用 `cursorTo` 并返回 `string`。 |
| `cursorToXY` | `(x: usize, y: usize, out: []u8): string` | 调用 `cursorToXY` 并返回 `string`。 |
| `cursorMove` | `(dx: i32, dy: i32, out: []u8): string` | 调用 `cursorMove` 并返回 `string`。 |
| `cursorUp` | `(n: usize, out: []u8): string` | 调用 `cursorUp` 并返回 `string`。 |
| `cursorDown` | `(n: usize, out: []u8): string` | 调用 `cursorDown` 并返回 `string`。 |
| `cursorForward` | `(n: usize, out: []u8): string` | 调用 `cursorForward` 并返回 `string`。 |
| `cursorBackward` | `(n: usize, out: []u8): string` | 调用 `cursorBackward` 并返回 `string`。 |
| `cursorLeft` | `(): string` | 调用 `cursorLeft` 并返回 `string`。 |
| `cursorSave` | `(): string` | 调用 `cursorSave` 并返回 `string`。 |
| `cursorRestore` | `(): string` | 调用 `cursorRestore` 并返回 `string`。 |
| `cursorHide` | `(): string` | 调用 `cursorHide` 并返回 `string`。 |
| `cursorShow` | `(): string` | 调用 `cursorShow` 并返回 `string`。 |
| `eraseLine` | `(): string` | 调用 `eraseLine` 并返回 `string`。 |
| `eraseEndLine` | `(): string` | 调用 `eraseEndLine` 并返回 `string`。 |
| `eraseStartLine` | `(): string` | 调用 `eraseStartLine` 并返回 `string`。 |
| `eraseDown` | `(): string` | 调用 `eraseDown` 并返回 `string`。 |
| `eraseUp` | `(): string` | 调用 `eraseUp` 并返回 `string`。 |
| `eraseScreen` | `(): string` | 调用 `eraseScreen` 并返回 `string`。 |
| `clearTerminal` | `(): string` | 调用 `clearTerminal` 并返回 `string`。 |
| `scrollUp` | `(): string` | 调用 `scrollUp` 并返回 `string`。 |
| `scrollDown` | `(): string` | 调用 `scrollDown` 并返回 `string`。 |
| `eraseLines` | `(count: usize, out: []u8): string` | 调用 `eraseLines` 并返回 `string`。 |
| `hyperlink` | `(text: string, url: string, out: []u8): string` | 调用 `hyperlink` 并返回 `string`。 |
| `setWindowTitle` | `(title: string, out: []u8): string` | 写入或更新状态数据。 |
| `beep` | `(): string` | 调用 `beep` 并返回 `string`。 |

## 最小调用示例

```cp
import { pushStr } from "ansi-escapes";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pushStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
