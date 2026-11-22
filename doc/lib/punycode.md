# cap:punycode

> 参考来源：/data/dev/bun/src/url.zig | 吸收日期：2026-11-22

基于 RFC 3492 Punycode / Bootstring 规范，提供纯 CP、零外部依赖、零堆分配的 Punycode 编解码器与 `xn--` 国际化域名标签转换库。

## 导入方式

```cp
import {
    isAsciiOnly,
    encodePunycode,
    decodePunycode,
    encodeIdnLabel,
    decodeIdnLabel,
} from "cap:punycode";
```

## 核心 API

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `isAsciiOnly` | `fn isAsciiOnly(s: []const u8): bool` | 判断是否仅含 ASCII 字符 |
| `encodePunycode` | `fn encodePunycode(utf8_label: []const u8, out: []u8): ([]const u8)?` | RFC 3492 §6.3 UTF-8 -> Punycode 编码 |
| `decodePunycode` | `fn decodePunycode(ascii_input: []const u8, out: []u8): ([]const u8)?` | RFC 3492 §6.2 Punycode -> UTF-8 解码（防溢出、拒代理项与非法字符） |
| `encodeIdnLabel` | `fn encodeIdnLabel(label: []const u8, out: []u8): ([]const u8)?` | 纯 ASCII 原样输出，含非 ASCII 输出 `xn--<punycode>` |
| `decodeIdnLabel` | `fn decodeIdnLabel(label: []const u8, out: []u8): ([]const u8)?` | 大小写无关识别 `xn--` 并解码，否则原样拷贝 |

## 使用示例

```cp
import { encodeIdnLabel, decodeIdnLabel } from "cap:punycode";

fn example() {
    let ace_buf: [128]u8 = undefined;
    let utf8_buf: [128]u8 = undefined;
    if (ace := encodeIdnLabel("münchen", &ace_buf)) {
        // "xn--mnchen-3ya"
        * = decodeIdnLabel(ace, &utf8_buf); // "münchen"
    }
}
```

## 上游行为契约与 IDN 安全边界

1. **防整数溢出与非法码点拦截（RFC 3492 §6.2）**：
   - `decodePunycode` 对 Bootstring 变长增量累加执行严格溢出检测，并拒绝解码出 UTF-16 代理项（`U+D800..U+DFFF`）、超 `U+10FFFF` 码点或含非 ASCII 基本码点的畸形输入，防止恶意域名触发越界写入。
2. **单标签粒度 (`encodeIdnLabel` / `decodeIdnLabel`)**：
   - IDNA Punycode 按 `.` 分隔的单个域名标签（label）分别编码而非整条 FQDN；纯 ASCII 标签由 `encodeIdnLabel` 原样透传，非 ASCII 标签自动加 `xn--` 前缀，`decodeIdnLabel` 大小写无关识别 `XN--`/`xn--`。
