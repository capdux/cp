# `cap:code39` — ISO/IEC 16388 Code 39 一维条码编解码与 Mod-43 校验库

零堆分配（Zero-Alloc）的纯 CP ISO/IEC 16388 Code 39（3-of-9）一维条码编解码库，完整实现 44 项标准 9 元素宽窄图案表、Mod-43 校验位计算与验证以及自适应窄/宽阈值 `0`/`1` 模块流编解码。

## 核心特性

- **零堆分配**：模块流编码与流式游程解码全程仅使用栈上固定数组与调用方输出缓冲。
- **完整 44 字符 3-of-9 图案表**：覆盖 `0..9`, `A..Z`, `- . SPACE $ / + %`（43 数据字符）及 `'*'` 起止符，每字符严格满足 9 元素（Bar/Space 交替）中恒有 3 个宽元素 + 6 个窄元素。
- **ISO/IEC 16388 Mod-43 校验**：支持独立计算、验证以及编解码链路自动附加/剥离校验位。
- **自适应游程解码**：支持 `wide_ratio = 2` 或 `3`（及任意整数倍上采样模块流），自动剥离前后静区 `0`，严格校验起止 `'*'` 哨兵与字符间白空。

## 导出 API

### 协议常量

| 常量 | 类型 | 值 | 说明 |
| --- | --- | --- | --- |
| `CODE39_ALPHABET_LEN` | `usize` | `43` | Code 39 数据字母表长度 |
| `CODE39_START_STOP` | `u8` | `'*'` (`0x2A`) | 起止哨兵字符 |
| `CODE39_ELEMENTS_PER_CHAR` | `usize` | `9` | 每字符条/空元素数 |
| `CODE39_WIDE_COUNT` | `usize` | `3` | 每字符宽元素数（3-of-9 不变量） |

### 字符表与 9-bit 图案映射

- `code39Value(ch: u8): u8?`：将 43 数据字符映射为 Mod-43 数值 `0..42`（不含 `'*'`）。
- `code39Char(val: u8): u8?`：将 `0..42` 逆映射为标准 ASCII 字符。
- `code39Pattern9(ch: u8): u16?`：返回字符（含 `'*'`）的 9-bit 宽窄位图（bit 8 为最左侧条，1=宽，0=窄）。
- `code39CharFromPattern9(pat9: u16): u8?`：从 9-bit 宽窄位图逆查字符（含 `'*'`）。

### Mod-43 与模块流编解码

- `code39CheckDigit(text: []const u8): u8?`：计算 `sum(value[i]) % 43` 对应的校验字符。
- `verifyCode39Mod43(text_with_check: []const u8): bool`：校验带校验位字符串的合法性。
- `encodeCode39Modules(text: []const u8, with_mod43: bool, wide_ratio: u8, out_modules: []u8): usize?`：编码为 `0`/`1` 模块流（自动加首尾 `'*'` 与 1 模块字符间隔，支持 `wide_ratio == 2 || 3`）。
- `decodeCode39Modules(modules: []const u8, with_mod43: bool, out_text: []u8): usize?`：解码 `0`/`1` 模块流，校验首尾 `'*'` 与可选 Mod-43 校验位并输出原始文本。

## 使用示例

```cp
import std from "std";
import {
    code39CheckDigit,
    encodeCode39Modules,
    decodeCode39Modules,
} from "cap:code39";

fn example() {
    let mods: [256]u8 = std.mem.zeroes([256]u8);
    let out: [64]u8 = std.mem.zeroes([64]u8);

    if (chk := code39CheckDigit("CODE39")) {
        // chk == 'W'
        * = chk;
    }

    if (mlen := encodeCode39Modules("CODE39", true, 3, mods[0..])) {
        if (tlen := decodeCode39Modules(mods[0..mlen], true, out[0..])) {
            // out[0..tlen] == "CODE39"
            * = tlen;
        }
    }
}
```
