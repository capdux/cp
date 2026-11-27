[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/utf8-valid

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:utf8parse` ([https://github.com/alacritty/vte](https://github.com/alacritty/vte)) / `crates.io:utf8_iter` ([https://github.com/hsivonen/utf8_iter](https://github.com/hsivonen/utf8_iter))（原库名 `utf8parse` → CP 包名 `@cap/utf8-valid`）

Zero-allocation RFC 3629 strict UTF-8 validator, WHATWG lossy repair (`U+FFFD`), ASCII fast-path, and UTF-16 code-unit length calculator for CP — clean-room MIT parity with Rust `utf-8` and `simdutf8`.

零堆分配的 CP RFC 3629 严格 UTF-8 验证与有损修复引擎，精准拦截过长编码（`0xC0/0xC1`, `0xE0 0x80..0x9F`, `0xF0 0x80..0x8F`）、UTF-16 代理区（`U+D800..U+DFFF`）、超 `U+10FFFF` 与末尾截断，支持 WHATWG 最大非法子段 `U+FFFD` 替换与 LSP/JS 互操作 `utf16Len` 计算（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/utf8-valid
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `Utf8Error` / `Utf8CheckResult` | `pub struct` | 精确报告 `valid`, `valid_up_to`, `error_len`（`0` 为末尾截断）, `codepoint_count` |
| `validateUtf8` | `(data: string) -> Utf8CheckResult` | RFC 3629 严格 UTF-8 校验 |
| `isValidUtf8` | `(data: string) -> bool` | 快速布尔校验 |
| `isAscii` | `(data: string) -> bool` | 纯 7-bit ASCII 快速检测 |
| `toLossyUtf8` | `(data: string, out: []u8) -> string` | WHATWG 标准有损修复（无效子段替换为 `U+FFFD`） |
| `utf16Len` | `(data: string) -> usize` | 计算等价 UTF-16 码元长度（BMP=1，辅助平面代理对=2） |

## Usage / 使用示例

```cp
import { validateUtf8, isValidUtf8, toLossyUtf8, utf16Len } from "@cap/utf8-valid";

const ok = isValidUtf8("Hello 世界 🚀"); // true
const u16 = utf16Len("A中🚀");          // 1 + 1 + 2 = 4

let bad: [4]u8 = ['O', 'K', 0xFF, '!'];
let out: [16]u8 = undefined;
const repaired = toLossyUtf8(bad[0..4], out[0..16]); // "OK\u{FFFD}!"
```

## License

MIT
