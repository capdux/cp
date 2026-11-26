# @cap/textwrap

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:wrap-ansi` ([https://github.com/chalk/wrap-ansi](https://github.com/chalk/wrap-ansi))（原库名 `wrap-ansi` → CP 包名 `@cap/textwrap`）

Zero-allocation ANSI-aware text wrapping, display width calculation, CJK wide-character breaking, `indent`/`dedent`, and display-width truncation for CP — clean-room MIT parity with npm `wrap-ansi` / `word-wrap` / `strip-indent` and Rust `textwrap`.

零堆分配的 CP 文本换行与排版引擎，完整支持 ANSI 彩色转义序列感知、UTF-8 与 CJK 全角宽字符列宽计算、按词边界换行、超长单词硬切分、`indent`/`dedent` 缩进处理以及按显示宽度截断（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/textwrap
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `WrapOptions` | `pub struct { width: usize, break_words: bool, preserve_newlines: bool }` | 换行配置结构体，提供 `WrapOptions.defaults()` 与 `WrapOptions.withWidth(width)` |
| `displayWidth` | `(s: string) -> usize` | 计算终端显示列宽（跳过 ANSI CSI/OSC 转义序列，CJK/全角/Emoji 计为 2 列） |
| `stripAnsi` | `(s: string, out: []u8) -> string` | 剥离全部 ANSI 转义序列，写入调用方缓冲区 |
| `wrapText` | `(text: string, width: usize, out: []u8) -> string` | 按默认配置（`break_words=true`, `preserve_newlines=true`）将文本折行到 `width` 列 |
| `wrapTextWith` | `(text: string, opts: WrapOptions, out: []u8) -> string` | 按自定义 `WrapOptions` 折行，支持词边界换行、CJK 任意字符边界换行及超长词硬断 |
| `indent` | `(text: string, prefix: string, out: []u8) -> string` | 为每个非空行追加 `prefix` 前缀缩进（保留空行） |
| `dedent` | `(text: string, out: []u8) -> string` | 计算所有非空行的公共最小前导空白并统一剥离（类似 Python `textwrap.dedent` / `strip-indent`） |
| `truncate` | `(text: string, max_width: usize, ellipsis: string, out: []u8) -> string` | 按显示列宽截断并追加省略号（支持 CJK 与 ANSI 安全重置） |

## Usage / 使用示例

```cp
import {
    WrapOptions,
    displayWidth,
    stripAnsi,
    wrapText,
    wrapTextWith,
    indent,
    dedent,
    truncate,
} from "@cap/textwrap";

let buf: [512]u8 = undefined;

// 1. Word wrapping & CJK wrapping
const wrapped = wrapText("Hello world from Capdux CP", 12, buf[0..512]);
// => "Hello world\nfrom Capdux\nCP"

const cjk = wrapText("你好世界和平", 6, buf[0..512]);
// => "你好世\n界和平"

// 2. ANSI-aware width & stripping
const colored = "\x1b[31mError\x1b[0m: 你好";
const cols = displayWidth(colored); // => 11 (5 + 2 + 4)
const plain = stripAnsi(colored, buf[0..512]); // => "Error: 你好"

// 3. Indent / Dedent round-trip
const indented = indent("line1\n\nline2", "    ", buf[0..512]);
// => "    line1\n\n    line2"
let buf2: [512]u8 = undefined;
const restored = dedent(indented, buf2[0..512]);
// => "line1\n\nline2"

// 4. Display-width truncation
const short = truncate("你好世界测试", 7, "...", buf[0..512]);
// => "你好..."
```

## License

MIT
