[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# base64url

纯 CP 实现的零堆分配 RFC 4648 Section 5 Base64URL（无填充与带填充）编解码器、校验器及标准 Base64 互转库。

- **包名**：`base64url`（`v1.0.0`）
- **上游对齐**：洁净室移植自 `npm/base64url`（`MIT`），底层复用 `data-encoding` 的 Base64 编解码原语与 `DecodeResult`。

## 安装与导入

```bash
cap add base64url
```

```typescript
import {
    encode, encodeBase64Url, encodePadded,
    decode, decodeBase64Url,
    encodedLen, encodedBase64UrlLen, encodedPaddedLen, decodedLen,
    fromBase64, fromStandardBase64, toBase64, toStandardBase64,
    padString, unpadString,
    validate, isValidBase64Url, validatePadded,
} from "base64url";
```

## 核心 API

### 编码与解码

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `encode` / `encodeBase64Url` | `(src: &[]u8, out: []u8): usize` | 将字节编码为无填充 Base64URL（`-` 与 `_`），返回写入字节数。 |
| `encodePadded` | `(src: &[]u8, out: []u8): usize` | 将字节编码为带 `=` 填充的 Base64URL，返回写入字节数。 |
| `decode` / `decodeBase64Url` | `(src: &[]u8, out: []u8): DecodeResult` | 解码带填充或无填充的 Base64URL 到 `out`，检查 `.ok`、`.len`、`.error_pos`。 |

### 长度预计算

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `encodedLen` / `encodedBase64UrlLen` | `(input_len: usize): usize` | 计算无填充 Base64URL 编码后的精确长度。 |
| `encodedPaddedLen` | `(input_len: usize): usize` | 计算带 `=` 填充的编码长度（`((n + 2) / 3) * 4`）。 |
| `decodedLen` | `(src: string): usize?` | 计算解码后的字节长度；若长度非法（`len % 4 == 1`）返回 `null`。 |

### 标准 Base64 互转、填充与校验

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `fromBase64` / `fromStandardBase64` | `(b64: string, out: []u8): usize` | 将标准 Base64（`+`, `/`, `=`）转换为无填充 Base64URL（`-`, `_`）。 |
| `toBase64` / `toStandardBase64` | `(b64url: string, out: []u8): usize` | 将 Base64URL 转换为带 `=` 填充的标准 Base64（`+`, `/`）。 |
| `padString` | `(s: string, out: []u8): usize` | 补齐尾部 `=` 使长度为 4 的倍数。 |
| `unpadString` | `(s: string): string` | 零拷贝去除尾部 `=` 填充字符。 |
| `validate` / `isValidBase64Url` | `(s: string): bool` | 校验无填充 Base64URL 字符集与长度合法性。 |
| `validatePadded` | `(s: string): bool` | 校验带 `=` 填充的 Base64URL 字符集、长度（`% 4 == 0`）与填充位置。 |

## 使用示例

### 1. 编解码 JWT 载荷

```typescript
import { encode, decode } from "base64url";

fn main() {
    let raw = "{\"sub\":\"42\"}";
    let enc: [64]u8 = undefined;
    let enc_len = encode(raw, enc[0..]);

    let dec: [64]u8 = undefined;
    let res = decode(enc[0..enc_len], dec[0..]);
    if (res.ok) {
        // dec[0..res.len] == raw
    }
}
```

### 2. 标准 Base64 与 Base64URL 互转

```typescript
import { fromBase64, toBase64, unpadString } from "base64url";

fn main() {
    let url_buf: [32]u8 = undefined;
    let n1 = fromBase64("ab+c/d==", url_buf[0..]); // "ab-c_d"

    let std_buf: [32]u8 = undefined;
    let n2 = toBase64("ab-c_d", std_buf[0..]);     // "ab+c/d=="
    let trimmed = unpadString("ab-c_d==");         // "ab-c_d"
}
```

### 3. 解码前预校验与长度计算

```typescript
import { validate, decodedLen } from "base64url";

fn main() {
    let token = "SGVsbG8td29ybGQ";
    if (validate(token)) {
        if (let out_len = decodedLen(token)) {
            // out_len == 11
        }
    }
}
```

## 与源包差异及零拷贝安全亮点

- **全程零堆分配**：所有编码、解码与格式转换函数直接写入调用方提供的 `out: []u8` 缓冲区，避免 JS `Buffer`/`String` 堆分配。
- **结构化解码诊断**：`decode` / `decodeBase64Url` 返回 `DecodeResult`（含 `ok`、`len`、`error_pos`），精确报告非法字符位置。
- **零拷贝去填充**：`unpadString` 直接返回原字符串子切片，无内存拷贝。
