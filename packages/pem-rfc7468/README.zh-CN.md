[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# pem-rfc7468

> **源包溯源**：吸收自 **crates.io** [`pem-rfc7468`](https://github.com/RustCrypto/formats)（`MIT OR Apache-2.0`），CP 官方洁净室实现 `pem-rfc7468@1.0.0`。

纯 CP 实现的零堆分配 RFC 7468 PEM 编码器与严格解码器。底层复用 `base64ct` 提供常量时间（Constant-Time）RFC 4648 Base64 编解码能力。

## 核心亮点

- **零堆分配**：帧边界解析、标签校验与 Base64 编解码全部在调用方缓冲区内完成。
- **零栈清零优化**：内部固定缓冲区（`PemLabel`、48 字节编码块、64 字节 Base64 行缓冲）采用 `undefined` 初始化，消除热路径上的冗余栈清零开销。
- **常量时间 Base64**：复用 `base64ct`，在编解码私钥 PEM 时避免分支与查表导致的时序侧信道。
- **严格 RFC 7468 校验**：自动跳过前置注释文本，校验 BEGIN/END 标签一致性、RFC 7468 §2 合法标签字符与规范 Base64 填充。

## 零成本迁移（Rust → CP）

直接沿用原包名 `pem-rfc7468`，无需修改包名：

**Rust：**
```rust
use pem_rfc7468::{encode, decode, LineEnding};
```

**CP：**
```typescript
import {
    PEM_OK, pemEncodedLen, encodePem, decodePem, decodeLabel, locatePem, isValidLabel
} from "pem-rfc7468";
```

## 实战示例

### 1. 将 DER 编码为 64 列换行的 PEM

```typescript
import { pemEncodedLen, encodePem, PEM_OK } from "pem-rfc7468";

export fn demoEncode(der: &[]u8): i64 {
    let need = pemEncodedLen(11, der.len, false); // "CERTIFICATE" 长度 11，LF 换行
    let mut out: [1024]u8 = undefined;
    if (need > 1024) {
        return -1;
    }
    let written = encodePem("CERTIFICATE", der, false, out[0..need]);
    if (written < PEM_OK) {
        return written;
    }
    return written;
}
```

### 2. 零拷贝定位块边界与严格解码

```typescript
import { decodePem, decodeLabel, locatePem } from "pem-rfc7468";

export fn demoDecode(pem_text: string): i64 {
    // 1. 零拷贝定位 PEM 块（自动跳过前置注释）
    let slice = locatePem(pem_text);
    if (!slice.isOk()) {
        return slice.err;
    }

    // 2. 仅提取头部标签，不解码 Base64 载荷
    let mut label_buf: [64]u8 = undefined;
    let label_len = decodeLabel(pem_text, label_buf[0..64]);
    if (label_len < 0) {
        return label_len;
    }

    // 3. 完整严格解码至 DER 缓冲区
    let mut out_label_len: usize = 0;
    let mut der_buf: [2048]u8 = undefined;
    return decodePem(pem_text, label_buf[0..64], &mut out_label_len, der_buf[0..2048]);
}
```

## 核心 API 速览

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `isValidLabel` | `(label: string): bool` | 校验 RFC 7468 §2 PEM 标签合法性 |
| `pemEncodedLen` | `(label_len: usize, der_len: usize, crlf: bool): usize` | 精确预计算编码后 PEM 字节长度 |
| `encodePem` | `(label: string, der: &[]u8, line_ending_crlf: bool, out: []u8): i64` | 编码 DER 为 64 列 PEM，返回写入字节数或负错误码 |
| `locatePem` | `(input: string): PemSlice` | 零拷贝定位首个 PEM 块边界（含 `end_pos` 可供流式切片） |
| `decodeLabel` | `(input: string, label_out: []u8): i64` | 仅提取头部标签，不执行 Base64 解码 |
| `decodePem` | `(input: string, label_out: []u8, label_len_out: &mut usize, der_out: []u8): i64` | 严格校验头尾标签并解码 DER |

### 错误码常量

- `PEM_OK = 0`（成功时函数返回 `>= 0` 的字节数）
- `PEM_ERR_INVALID_LABEL = -1` / `PEM_ERR_BUFFER_TOO_SMALL = -2`
- `PEM_ERR_MISSING_BEGIN = -3` / `PEM_ERR_MISSING_END = -4` / `PEM_ERR_LABEL_MISMATCH = -5`
- `PEM_ERR_INVALID_BASE64 = -6` / `PEM_ERR_INVALID_HEADER = -7` / `PEM_ERR_INVALID_FOOTER = -8`

## 许可证

MIT
