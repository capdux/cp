# @cap/wrap-ansi

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:wrap-ansi` ([https://github.com/chalk/wrap-ansi](https://github.com/chalk/wrap-ansi))

零堆分配的 CP ANSI 颜色保持折行与按可见列宽截断引擎 — 洁净室 MIT 实现，对标 npm `wrap-ansi`（6.15 亿/周）与 `slice-ansi`（5.5 亿/周）。

Zero-allocation ANSI-preserving text wrapper and visible-column slicer for CP — clean-room MIT parity with npm `wrap-ansi` (615M/wk) and `slice-ansi` (550M/wk).

## 特性 / Features

- **精确可见宽度 / Visible Width**：`visibleWidth` 自动忽略 ANSI CSI 序列（`\x1b[...m`），ASCII 计宽 1，CJK/全角/Emoji 计宽 2。
- **ANSI 样式跨行延续 / ANSI Style Continuity**：`wrapAnsi` 实时追踪激活的 SGR 样式栈，在单词边界或 `hard_wrap` 字符边界折行前自动追加 `\x1b[0m` 关闭样式，换行后自动重新注入当前样式，彻底避免终端颜色泄漏或断色。
- **按可见列安全切片 / Visible Column Slicing**：`sliceAnsi` 按 `[start_col, end_col)` 截取内容，自动补全进入切片前已激活的 ANSI 样式并在末尾安全闭合 `\x1b[0m`。
- **零堆分配 / Zero Heap Allocation**：全部写入调用方 `out: []u8` 缓冲区。

## 安装 / Install

```bash
cap add @cap/wrap-ansi
```

## API 参考 / API Reference

| 函数 / Function | 签名 / Signature | 说明 / Description |
| --- | --- | --- |
| `visibleWidth` | `(s: string) -> usize` | 计算忽略 ANSI 转义序列后的终端显示列宽 |
| `wrapAnsi` | `(text: string, max_cols: usize, hard_wrap: bool, out: []u8) -> string` | 按列折行并跨行保持 ANSI SGR 样式完整 |
| `sliceAnsi` | `(text: string, start_col: usize, end_col: usize, out: []u8) -> string` | 按可见列区间 `[start_col, end_col)` 安全切片并保持 ANSI 样式 |

## 使用示例 / Usage

```cp
import { visibleWidth, wrapAnsi, sliceAnsi } from "@cap/wrap-ansi";

const w = visibleWidth("\x1b[31mHello 世界\x1b[0m"); // 10

let buf: [512]u8 = undefined;
const wrapped = wrapAnsi("\x1b[31mHello World\x1b[0m", 7 as usize, false, buf[0..512]);
// => "\x1b[31mHello\x1b[0m\n\x1b[31mWorld\x1b[0m"

const sliced = sliceAnsi("\x1b[32mHello World\x1b[0m", 6 as usize, 11 as usize, buf[0..512]);
// => "\x1b[32mWorld\x1b[0m"
```

## License

MIT
