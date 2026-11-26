# @cap/html-entities

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:he` ([https://github.com/mathiasbynens/he](https://github.com/mathiasbynens/he))（原库名 `he` → CP 包名 `@cap/html-entities`）

零堆分配的 HTML5 命名字符实体与数字字符引用 UTF-8 编解码引擎 — 洁净室 MIT 实现，对标 npm `he` / `escape-html` / `html-entities` 与 Rust `html-escape`。

Zero-allocation HTML5 named entity & numeric character reference UTF-8 encoder/decoder and XSS escaper for CP — clean-room MIT parity with npm `he`, `escape-html`, `html-entities`, and Rust `html-escape`.

## 特性 / Features

- **XSS 安全转义 / XSS-safe escaping**：`escapeHtml` 精准转义 `&`、`<`、`>`、`"`、`'` 为 `&amp;`、`&lt;`、`&gt;`、`&quot;`、`&#39;`，附带 `needsHtmlEscape` 快速预检。
- **全 Unicode 非 ASCII 编码 / Non-ASCII encoding**：`encodeNonAscii` 将所有非 ASCII UTF-8 码点（中文、Emoji 等）编码为标准 `&#xHEX;` 引用。
- **HTML5 完整解码 / Full HTML5 decoding**：支持十进制 `&#128640;`、十六进制 `&#x1F680;` / `&#X1f680;` 与 60+ 高频命名实体（`&nbsp;`, `&copy;`, `&euro;`, `&mdash;`, `&rarr;`, `&hearts;` 等），自动将非法/代理区码点替换为 `U+FFFD`，未知实体与孤立 `&` 原样安全保留。
- **零堆分配 / Zero heap allocation**：全部写入调用方 `out: []u8` 缓冲区。

## 安装 / Install

```bash
cap add @cap/html-entities
```

## API 参考 / API Reference

| 函数 / Function | 签名 / Signature | 说明 / Description |
| --- | --- | --- |
| `escapeHtml` | `(input: string, out: []u8) -> string` | 转义 `&`, `<`, `>`, `"`, `'` 防 XSS |
| `encodeNonAscii` | `(input: string, out: []u8) -> string` | 转义 5 基础字符并将所有非 ASCII UTF-8 编码为 `&#xHEX;` |
| `decodeHtmlEntities` | `(input: string, out: []u8) -> string` | 解码 HTML5 命名实体与十进制/十六进制数字引用为 UTF-8 |
| `needsHtmlEscape` | `(s: string) -> bool` | 快速检查字符串是否包含需要转义的 HTML 特殊字符 |

## 使用示例 / Usage

```cp
import {
    escapeHtml,
    encodeNonAscii,
    decodeHtmlEntities,
    needsHtmlEscape,
} from "@cap/html-entities";

let buf: [512]u8 = undefined;
const safe = escapeHtml("<script>alert(\"x\" & 'y')</script>", buf[0..512]);
// => "&lt;script&gt;alert(&quot;x&quot; &amp; &#39;y&#39;)&lt;/script&gt;"

const decoded = decodeHtmlEntities("CP &copy; 2026 &mdash; &#x1F680; &hearts;", buf[0..512]);
// => "CP © 2026 — 🚀 ♥"

const encoded = encodeNonAscii("Hello 世界 🚀", buf[0..512]);
// => "Hello &#x4E16;&#x754C; &#x1F680;"
```

## License

MIT
