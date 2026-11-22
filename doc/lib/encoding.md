# CP 标准库：二进制与文本编码子系统开发与使用手册 (`cap:encoding`)

> 参考来源：`/data/dev/bun/src/base64/base64.cp`, `/data/dev/bun/src/string/wtf.cp`, `/data/dev/bun/docs/runtime/utils.mdx` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:encoding` 是 CP 语言官方内置的工业级二进制与文本编码转换标准库，全面吸收自 Bun 生产级运行时源码（`/data/dev/bun/src/base64/base64.cp` 及字符编码基础设施）。

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        CP 用户层应用 (Application)                      │
├───────────────────────────────────┬────────────────────────────────────┤
│       Base64 编解码子系统          │         Hex 十六进制子系统         │
│  Base64 / Codecs / Base64Encoder  │  Hex.encodeLower / Hex.encodeUpper │
│  Base64Decoder / decodeInPlace    │  Hex.decode / Hex.decodeInPlace    │
├───────────────────────────────────┴────────────────────────────────────┤
│               Unicode & UTF 码点子系统 (Utf8 / Utf16 / Width)          │
├────────────────────────────────────────────────────────────────────────┤
│       零堆分配栈缓冲快路径 (Stack Slice)  +  动态分配器 (Allocator)     │
└────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

`lib/encoding/` 下所有源文件第 1 行均严格标记上游 Bun 源码来源：

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/encoding/base64_.cp` | `// Source: /data/dev/bun/src/base64/base64.cp` | 底层 Base64 字符集、单字节解码映射、`Base64Encoder`、`Base64Decoder`、`Codecs` 预设实例 |
| `lib/encoding/base64.cp` | `// Source: /data/dev/bun/src/base64/base64.cp` | 面向用户的 `Base64` 高阶静态类（标准/URL-Safe、预分配/动态分配、原地解码、合法性校验） |
| `lib/encoding/hex.cp` | `// Source: /data/dev/bun/src/base64/base64.cp` | 零堆分配查表驱动的 `Hex` 十六进制大小写编码、解码、原地解码与合法性校验 |
| `lib/encoding/utf16be.cp` | `// Source: /data/dev/bun/src/string/wtf.cp` | `Utf16BE` 大端 UTF-16 编解码与 `Ascii`（大小写转换、Latin-1 转 UTF-8）原语 |
| `lib/encoding/index.cp` | `// Source: /data/dev/bun/src/base64/base64.cp` | `cap:encoding` 标准库统一导出入口（聚合 Base64、Hex、UTF-8/UTF-16LE/UTF-16BE、Ascii 与 PercentEncoding） |

### 2. 核心设计特性与性能优势

1. **双模式内存策略（Pre-allocated Zero-Heap vs Dynamic Allocator）**：
   - **预分配栈/池模式**：`Base64.encode`、`Base64.decode`、`Hex.encodeLower`、`Hex.decode` 直接操作调用方提供的 `[]u8` 缓冲区，100% 零堆内存分配（Zero Heap Allocation），适合百万级并发 Actor 高频协议包序列化；
   - **动态分配器模式**：`Base64.encodeAlloc`、`Base64.decodeAlloc`、`Hex.encodeAlloc`、`Hex.decodeAlloc` 接受 `std.mem.Allocator`，自动计算精确容量并返回堆切片，内建 `errdefer` 异常释放保护。
2. **原地零拷贝解码（In-Place Decoding）**：
   - 由于 Base64（4 字节编码 -> 3 字节原文）和 Hex（2 字节编码 -> 1 字节原文）解码后的字节数严格小于编码长度，`Base64.decodeInPlace` 与 `Hex.decodeInPlace` 支持在同一块 `[]u8` 缓冲区内边读边写覆盖，峰值额外内存开销为 **0 字节**。
3. **统一宽容解码器（Unified Forgiving Decoder）**：
   - `Base64.decode` 与 `Base64.decodeInPlace` 同时兼容标准字符集（`+`、`/`）与 URL-Safe 字符集（`-`、`_`），兼容有填充（`=`）与无填充输入，并自动跳过 ASCII 空白字符（`' '`、`'\t'`、`'\r'`、`'\n'`），符合 WHATWG Forgiving-Base64 规范。

---

## 二、 Base64 编解码详尽指南

### 1. 标准 Base64 与 URL-Safe Base64（预分配缓冲区 vs 动态分配）

```typescript
import {
    Base64,
    standard,
    standard_no_pad,
    url_safe,
    url_safe_no_pad,
} from "cap:encoding";
const std = @import("std");

pub fn demoBase64(): !void {
    const payload = "Concurrent Plus (CP) Native Runtime";

    // =========================================================================
    // A. 预分配栈缓冲区（零堆分配）
    // =========================================================================
    let enc_buf: [128]u8 = undefined;
    let dec_buf: [128]u8 = undefined;

    // 1. 标准带 '=' 填充编码 (RFC 4648 §4)
    const enc_len = Base64.encode(&enc_buf, payload);
    const encoded_slice = enc_buf[0..enc_len];

    // 2. 预分配解码
    const dec_len = Base64.decode(&dec_buf, encoded_slice)?;
    const decoded_slice = dec_buf[0..dec_len];

    // 3. URL-Safe 无填充编码 (RFC 4648 §5, 适用于 JWT / URL Token)
    let url_buf: [128]u8 = undefined;
    const url_len = Base64.encodeURLSafe(&url_buf, "\xFB\xFF\xFE\x00\x3F");
    // 输出使用 '-' 和 '_' 替代 '+' 和 '/'，且不含末尾 '='

    // =========================================================================
    // B. 动态堆分配（Allocator 驱动）
    // =========================================================================
    const alloc = std.heap.page_allocator;

    // 1. 动态标准 Base64 编解码
    const heap_enc = Base64.encodeAlloc(alloc, payload)?;
    defer alloc.free(heap_enc);

    const heap_dec = Base64.decodeAlloc(alloc, heap_enc)?;
    defer alloc.free(heap_dec);

    // 2. 动态 URL-Safe Base64 编解码
    const heap_url_enc = Base64.encodeURLSafeAlloc(alloc, payload)?;
    defer alloc.free(heap_url_enc);

    const heap_url_dec = Base64.decodeURLSafeAlloc(alloc, heap_url_enc)?;
    defer alloc.free(heap_url_dec);
}
```

### 2. 原地零内存解码 (`Base64.decodeInPlace`)

```typescript
import { Base64 } from "cap:encoding";

pub fn demoInPlace(): !void {
    let packet_buf: [16]u8 = undefined;
    @memcpy(packet_buf[0..12], "SGVsbG8gQ1Ah"); // "Hello CP!"

    // 直接在原缓冲区内解码覆盖，无需分配第二个目标数组
    const n = Base64.decodeInPlace(packet_buf[0..12])?;
    // packet_buf[0..n] === "Hello CP!" (n == 9)
}
```

### 3. 四种预设编解码器实例 (`Codecs`)

当需要严格区分填充策略与字符集时，可直接使用预设的 `Codecs` 实例：

| Codec 常量 | 字符集 (`62`, `63`) | 填充字符 (`pad_char`) | 适用场景 |
| :--- | :---: | :---: | :--- |
| `standard` | `+`, `/` | `'='` | 标准 MIME / HTTP Basic Auth / PEM 证书 (RFC 4648 §4) |
| `standard_no_pad` | `+`, `/` | `null` | 紧凑二进制流传输（省略末尾 `=`） |
| `url_safe` | `-`, `_` | `'='` | 需要对齐 4 字节倍数的文件名与 URL 参数 |
| `url_safe_no_pad` | `-`, `_` | `null` | JWT (JSON Web Token)、OAuth2 PKCE `code_challenge` (RFC 4648 §5) |

---

## 三、 Hex 十六进制编解码详尽指南

`Hex` 采用紧凑查表法实现字节流与大小写十六进制字符串的极速互转：

```typescript
import { Hex } from "cap:encoding";
const std = @import("std");

pub fn demoHex(): !void {
    const raw = "CP-2026!";

    // 1. 合法性与长度计算
    const enc_needed = Hex.encodedLen(raw.len); // 16
    const dec_needed = Hex.decodedLen(enc_needed); // 8

    // 2. 栈缓冲区小写/大写编码（零堆分配）
    let lower_buf: [32]u8 = undefined;
    const lower_len = Hex.encodeLower(raw, &lower_buf);
    // lower_buf[0..lower_len] => "43502d3230323621"

    let upper_buf: [32]u8 = undefined;
    const upper_len = Hex.encodeUpper(raw, &upper_buf);
    // upper_buf[0..upper_len] => "43502D3230323621"

    // 3. 校验与解码（支持大小写混编输入）
    if (Hex.isValid(upper_buf[0..upper_len])) {
        let out_buf: [16]u8 = undefined;
        const n = Hex.decode(upper_buf[0..upper_len], &out_buf)?;
        // out_buf[0..n] => "CP-2026!"
    }

    // 4. 原地解码 (In-Place)
    const in_place_len = Hex.decodeInPlace(lower_buf[0..lower_len])?;
    // lower_buf[0..in_place_len] => "CP-2026!"

    // 5. 动态分配编解码
    const alloc = std.heap.page_allocator;
    const heap_hex = Hex.encodeAlloc(alloc, raw)?;
    defer alloc.free(heap_hex);

    const heap_raw = Hex.decodeAlloc(alloc, heap_hex)?;
    defer alloc.free(heap_raw);
}
```

---

## 四、 与 Bun / Node.js `Buffer` API 对照表

| 功能分类 | Node.js (`Buffer`) | Bun (`src/base64` & `Buffer`) | CP 标准库 (`cap:encoding`) |
| :--- | :--- | :--- | :--- |
| **Base64 标准编码 (动态)** | `Buffer.from(src).toString("base64")` | `Base64.encodeAlloc(alloc, src)` | `Base64.encodeAlloc(alloc, src)` |
| **Base64 标准编码 (预分配)** | `buf.write(src, "base64")` | `Base64.encode(dest, src)` | `Base64.encode(dest, src)` |
| **Base64 URL-Safe 编码** | `Buffer.from(src).toString("base64url")` | `Base64.encodeURLSafe(dest, src)` | `Base64.encodeURLSafe(dest, src)` / `Base64.encodeURLSafeAlloc(alloc, src)` |
| **Base64 解码 (动态)** | `Buffer.from(b64, "base64")` | `Base64.decodeAlloc(alloc, b64)` | `Base64.decodeAlloc(alloc, b64)` |
| **Base64 解码 (预分配)** | `Buffer.from(b64, "base64").copy(dest)` | `Base64.decode(dest, b64)` | `Base64.decode(dest, b64)` |
| **Base64 原地解码** | *不支持（需新建 Buffer）* | *通过 slice 覆写* | `Base64.decodeInPlace(buf)` |
| **Base64 编码长度计算** | `Math.ceil(len / 3) * 4` | `Base64.encodeLenFromSize(len)` | `Base64.encodeLen(len)` / `Base64.encodeLenURLSafe(len)` |
| **Base64 解码精确长度** | `Buffer.byteLength(b64, "base64")` | `standard.Decoder.calcSizeForSlice(b64)` | `Base64.exactDecodedLen(b64)` |
| **Hex 编码 (小写/大写)** | `Buffer.from(src).toString("hex")` | `std.fmt.bytesToHex(src, .lower)` | `Hex.encodeLower(src, out)` / `Hex.encodeUpper(src, out)` / `Hex.encodeAlloc(alloc, src)` |
| **Hex 解码** | `Buffer.from(hex, "hex")` | `std.fmt.hexToBytes(out, hex)` | `Hex.decode(hex, out)` / `Hex.decodeInPlace(buf)` / `Hex.decodeAlloc(alloc, hex)` |
| **Hex 合法性校验** | `/^[0-9a-fA-F]+$/` (偶数长度) | 手动遍历 nibble | `Hex.isValid(hex)` |

---

## 五、 上游 Bun 行为契约与边界注意事项

1. **Base64 宽容解码 vs 严格编解码器选择**：
   - `Base64.decode` / `Base64.decodeInPlace` 遵循 WHATWG Forgiving-Base64 与 Bun `src/base64/base64.cp` 约定：同时接受标准表（`+`/`/`）与 URL-Safe 表（`-`/`_`）、允许省略末尾 `=` 填充并自动忽略 ASCII 空白符；若协议要求严格校验字符集与填充，请直接使用 `Codecs` 中的 `standard`、`standard_no_pad`、`url_safe`、`url_safe_no_pad` 及 `Base64Decoder`。
2. **输出缓冲区容量精确预估**：
   - 调用零堆分配编码接口前，请先使用 `Base64.encodeLen(src.len)` / `Base64.encodeLenURLSafe(src.len)` 或 `Base64.exactDecodedLen(b64)` 计算所需字节数；Hex 编码输出长度恒为 `2 * src.len`，解码输出长度为 `hex.len / 2`（奇数长度或非法十六进制字符会被 `Hex.isValid` 与 `Hex.decode` 拦截）。
3. **UTF-8 / UTF-16 / Ascii 跨边界替换策略**：
   - 聚合导出的 `Utf8`、`Utf16`、`Utf16BE` 遇到非法代理项或截断多字节序列时统一替换为 `REPLACEMENT_CHAR`（`U+FFFD`），避免未校验外部字节流引发越界读取；`Ascii` 原语针对纯 ASCII/Latin-1 快路径优化，零堆分配完成大小写转换与 Latin-1 到 UTF-8 展开。
4. **百分号编码复用**：
   - `cap:encoding` 直接重导出 `PercentEncoding`（别名 `Percent`），适合在同一模块内同时处理 Base64/Hex/UTF 与 URI 百分号编解码而无需额外引入 `cap:url`。

### 6. WHATWG 编码标签规范化与 BOM 探测 (`encodingLabel.cp`)

> 吸收自 `/data/dev/bun/src/runtime/webcore/EncodingLabel.cp`（参考 WHATWG Encoding Standard §4.2 及 `/data/dev/bun/docs/runtime/web-apis.mdx` 中 `TextDecoder` 编码标签规范）

- **导出符号**：`EncodingLabel`（24 种 WHATWG 标准编码枚举；方法 `getLabel`/`isUnicode`/`isCjk`/`isSingleByte`/`isAsciiCompatible`/`which`）、`lookupEncodingLabel`、`canonicalEncodingName`、`detectBomEncoding`；
- **关键行为契约**：
  - 自动裁剪首尾 WHATWG ASCII 空白（`" \t\r\n\x0C"`）并执行大小写不敏感查表，覆盖 135+ 标准别名（如 `latin1`/`us-ascii`/`iso-8859-1` 归一为 `windows-1252`，`gb2312`/`chinese` 归一为 `gbk`，`ms932`/`windows-31j` 归一为 `shift_jis`，`utf-16`/`ucs-2` 归一为 `utf-16le`）；
  - `detectBomEncoding(bytes)` 零分配识别 UTF-8 (`EF BB BF`)、UTF-16BE (`FE FF`) 与 UTF-16LE (`FF FE`) 字节序标记。

```cp
import { EncodingLabel, canonicalEncodingName, detectBomEncoding } from "cap:encoding";

const lbl = EncodingLabel.which("  US-ASCII "); // EncodingLabel.Windows1252
const name = canonicalEncodingName("ms932");    // "shift_jis"
const bom_bytes: [3]u8 = [0xEF, 0xBB, 0xBF];
const bom = detectBomEncoding(bom_bytes[0..]);  // EncodingLabel.Utf8
* = lbl; * = name; * = bom;
```
