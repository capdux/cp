// Source: cp/doc/lib/unicode.md
# `cap:unicode` — Unicode / UTF-8 / UTF-16 / Grapheme 与终端显示宽度标准库

纯 CP、零 C/Posix 依赖的工业级 Unicode 基础库，提供 RFC 3629 UTF-8 与 UTF-16 代理对严格编解码、Unicode 码点属性与大小写映射、UAX #29 Extended Grapheme Cluster 零分配分割迭代器，以及 East Asian Width / Emoji 终端显示宽度计算与安全截断。

## 导入方式

```cp
import {
    UnicodeError,
    Utf8Step,
    isValidUtf8,
    countCodepoints,
    decodeUtf8Step,
    encodeUtf8,
    utf8ToUtf16,
    utf16ToUtf8,
    isWhitespace,
    isControl,
    isAscii,
    isIdentifierStart,
    isIdentifierContinue,
    toLowerCodepoint,
    toUpperCodepoint,
    toLowerUtf8,
    toUpperUtf8,
    GraphemeBreakClass,
    GraphemeSpan,
    GraphemeIterator,
    countGraphemes,
    codepointWidth,
    displayWidth,
    truncateByWidth,
} from "cap:unicode";

// 或动态命名空间挂载：
import cap from "cap";
// cap.unicode.isValidUtf8 / cap.unicode.GraphemeIterator / cap.unicode.displayWidth
```

## 核心类型与错误

### `UnicodeError`
- `InvalidUtf8`：遇到非法 UTF-8 字节序列（超长编码、代理区码点、孤立续字节或截断序列）
- `InvalidUtf16`：非法 UTF-16 编码单元
- `UnpairedSurrogate`：UTF-16 中出现未配对的高代理（`0xD800..0xDBFF`）或低代理（`0xDC00..0xDFFF`）
- `InvalidCodepoint`：超出 `0x10FFFF` 或落在代理区 `0xD800..0xDFFF` 的码点
- `BufferTooSmall`：输出缓冲区空间不足
- `OutOfMemory`：内存分配失败

### `Utf8Step`
- `codepoint: u32`：解码得到的 Unicode 标量值
- `len: u8`：消耗的 UTF-8 字节数（`1..4`）
- `valid: bool`：是否为合法 UTF-8 序列

### `GraphemeSpan` / `GraphemeIterator`
- `GraphemeSpan`：`{ start: usize, end: usize }`，提供 `len(): usize` 与 `slice(src: []const u8): []const u8`
- `GraphemeIterator.init(bytes: []const u8): GraphemeIterator`
- `nextSpan(*_): GraphemeSpan?` / `next(*_): ([]const u8)?` / `peek(*_): ([]const u8)?` / `reset(*_)`

## 核心 API

### 1. UTF-8 / UTF-16 编解码与校验
- `isValidUtf8(bytes: []const u8): bool`：含 8 字节 ASCII 快速路径与 RFC 3629 全量校验
- `countCodepoints(bytes: []const u8): usize`：统计 UTF-8 码点数量
- `decodeUtf8Step(bytes: []const u8, offset: usize): Utf8Step ! UnicodeError`：单步严格解码 UTF-8
- `encodeUtf8(cp: u32, out: []u8): usize ! UnicodeError`：将标量值编码为 UTF-8 字节
- `utf8ToUtf16(alloc, bytes: []const u8): ([]u16) ! UnicodeError`：UTF-8 转 UTF-16（支持增补平面代理对）
- `utf16ToUtf8(alloc, units: []const u16): ([]u8) ! UnicodeError`：UTF-16 转 UTF-8（严格拦截非配对代理）

### 2. Unicode 码点属性与大小写映射
- `isAscii(cp: u32): bool` / `isControl(cp: u32): bool` / `isWhitespace(cp: u32): bool`
- `isIdentifierStart(cp: u32): bool` / `isIdentifierContinue(cp: u32): bool`
- `toLowerCodepoint(cp: u32): u32` / `toUpperCodepoint(cp: u32): u32`
- `toLowerUtf8(alloc, bytes: []const u8): ([]u8) ! UnicodeError`
- `toUpperUtf8(alloc, bytes: []const u8): ([]u8) ! UnicodeError`

### 3. UAX #29 Grapheme 分割与显示宽度
- `countGraphemes(s: []const u8): usize`：统计用户感知字符簇数量（支持 `\r\n`、Combining Marks、ZWJ Emoji、Regional Indicator 国旗配对、Hangul Jamo、Skin Tone）
- `codepointWidth(cp: u32): u8`：返回单码点显示宽度（`0`、`1` 或 `2`）
- `displayWidth(s: []const u8): usize`：计算终端等宽显示列数（自动跳过 ANSI CSI/OSC 转义序列，按 Grapheme 簇计宽）
- `truncateByWidth(alloc, s: []const u8, max_width: usize, ellipsis: []const u8): ([]u8) ! UnicodeError`：按显示宽度安全截断并追加省略号，绝不切碎多字节或 Grapheme 簇


---

## 收敛扩展子模块（`lut.cp` / `range.cp` / `wcwidth.cp`）

- **三级压缩查找表 (`lut.cp`)**：`BLOCK_SIZE`, `MAX_UNICODE_CODEPOINT`, `MAX_UNIQUE_ELEMENTS`, `Error`, `Tables<Elem>`, `Generator<Elem, Context>`，实现 O(1) Stage 1 -> Stage 2 -> Stage 3 Unicode 属性压缩表生成与查询。
- **CSS `@font-face` Unicode 区间 (`range.cp`)**：`MAX_CODEPOINT`, `UnicodeRangeError`, `UnicodeRange`（`U+XXXX` / `U+XXXX-YYYY` / `U+??` 通配符区间解析、规范化与格式化）。
- **终端列宽扩展 (`wcwidth.cp`)**：`wcwidth`（原 `codepointWidth` 别名）、`isZeroWidth`、`isWideChar`、`strDisplayWidth`、`sliceToDisplayWidth`。
