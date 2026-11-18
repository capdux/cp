# `cap:encoding` — 高性能二进制与文本编码标准库

`cap:encoding` 汇集了 CP 原生高性能 Base64（标准/URL-Safe/无填充/原地解码）、十六进制 Hex、以及 Unicode / UTF-8 / UTF-16 编码转换能力，全面对标 Bun 原生编码基础设施。

## 核心组件与特性

1. **工业级 Base64 编解码引擎**：
   - 支持标准（`standard`）与 URL-safe（`url_safe`）字符集；
   - 支持带填充（RFC 4648）与无填充（No-Padding）；
   - 提供快速原地解码 `decodeInPlace`（零内存分配）；
   - 包含多码流编解码器工厂 `Codecs`。
2. **十六进制 Hex 转换**：
   - 大小写十六进制字符串与原始字节流互转；
   - 严格边界校验与快速查表。
3. **Unicode / UTF 编码处理**：
   - `Utf8` / `Utf16` 码点与字节转换；
   - `UnicodeWidth` 终端字符宽度计算；
   - 畸形编码安全替换符（`REPLACEMENT_CHAR = 0xFFFD`）。

---

## API 使用示例

### 1. Base64 基础编解码

```typescript
import { Base64 } from "cap:encoding";
import std from "std";

// 计算最大输出缓冲长度
const src = "Hello CP!";
const max_encoded = Base64.encodeLen(src.len);

// 动态分配编码
let encoded = Base64.encodeAlloc(std.heap.page_allocator, src)?;
// encoded => "SGVsbG8gQ1Ah"

// 动态分配解码
let decoded = Base64.decodeAlloc(std.heap.page_allocator, encoded)?;
assert(decoded == src);

std.heap.page_allocator.free(encoded);
std.heap.page_allocator.free(decoded);
```

### 2. URL-Safe 与无填充编码

```typescript
import { Codecs, standard_no_pad, url_safe_no_pad } from "cap:encoding";

// URL-safe 无填充（常用于 JWT、OAuth2）
let url_codec = url_safe_no_pad;
let dest: [64]u8 = undefined;
let written = url_codec.encode(&dest, "user_token_payload");
```

### 3. 原地极速解码（零堆分配）

```typescript
import { Base64Decoder } from "cap:encoding";

let buffer: [32]u8 = undefined;
const b64_str = "QW50aWdyYXZpdHk=";
@memcpy(buffer[0..b64_str.len], b64_str);

// 直接在当前缓冲区原地解算并覆盖
let actual_len = Base64Decoder.decodeInPlace(buffer[0..b64_str.len])?;
assert(buffer[0..actual_len] == "Antigravity");
```

---

## 溯源关系
本模块代码完全吸收自 Bun 生产源码：
- `lib/encoding/base64.cp`: `// Source: bun/src/base64/base64.cp`
- `lib/encoding/hex.cp`: `// Source: Bun src/string/`
- `lib/encoding/index.cp`: `// Source: bun/src/base64/base64.cp`
