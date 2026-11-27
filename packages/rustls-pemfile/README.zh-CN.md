[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# rustls-pemfile

> **源包溯源**：吸收自 **crates.io** [`rustls-pemfile`](https://github.com/rustls/pemfile)（v2 / `MIT OR Apache-2.0`），CP 官方洁净室实现 `rustls-pemfile@1.0.0`。

纯 CP 实现的零堆分配流式 TLS PEM 多块读取器。底层直接复用 `pem-rfc7468` 完成 RFC 7468 帧定界、标签校验与常量时间 Base64 解码。

## 核心亮点

- **零堆分配流式读取**：`PemBundleReader` 仅维护 `(input, offset)` 游标，将每个 PEM 块直接解码至调用方提供的 `der_out: []u8` 缓冲区。
- **零栈清零优化**：内部 64 字节标签暂存缓冲区使用 `undefined` 初始化，避免每次 `nextItem` / `peekNextKind` 调用产生多余的栈清零开销。
- **完整 TLS 对象分类**：自动识别 X.509 证书、PKCS#8 / PKCS#1 / SEC1 私钥、SPKI 公钥、CRL 与 CSR。
- **免解码预扫描**：`countItems()`、`countCerts()` 与 `peekNextKind()` 仅扫描边界与标签，不执行 Base64 解码。

## 零成本迁移（Rust → CP）

直接沿用原包名 `rustls-pemfile`，无需改名：

**Rust：**
```rust
use rustls_pemfile::{certs, private_key, read_one, Item};
```

**CP：**
```typescript
import {
    PemBundleReader, PemItem, readFirstPrivateKey,
    classifyPemLabel, isPrivateKeyKind,
    PEM_ITEM_X509_CERT, PEM_ITEM_PKCS8_KEY, PEM_ITEM_PKCS1_KEY, PEM_ITEM_SEC1_KEY
} from "rustls-pemfile";
```

## 实战示例

### 1. 流式遍历证书链与私钥混合包

```typescript
import { PemBundleReader } from "rustls-pemfile";

export fn loadTlsBundle(pem_bundle: string): usize {
    let mut reader = PemBundleReader.new(pem_bundle);
    let mut der_buf: [4096]u8 = undefined;
    let mut cert_count: usize = 0;

    while (!reader.isEof()) {
        let item = reader.nextItem(der_buf[0..4096]);
        if (item.eof) {
            break;
        }
        if (!item.ok) {
            return 0; // PEM 格式非法或输出缓冲不足
        }
        if (item.isCert()) {
            let cert_der = der_buf[0..item.der_len];
            cert_count += 1;
            _ = cert_der;
        } else if (item.isPrivateKey()) {
            let key_der = der_buf[0..item.der_len];
            _ = key_der;
        }
    }
    return cert_count;
}
```

### 2. 从混合 PEM 包中提取首个私钥

```typescript
import { readFirstPrivateKey } from "rustls-pemfile";

export fn loadPrivateKey(bundle: string, out: []u8): usize {
    let item = readFirstPrivateKey(bundle, out);
    if (item.ok && item.isPrivateKey()) {
        return item.der_len;
    }
    return 0;
}
```

## 核心 API 速览

### 对象类型常量 (`u8`)

- `PEM_ITEM_NONE (0)`：无对象 / EOF / 非法标签
- `PEM_ITEM_X509_CERT (1)`：`CERTIFICATE`
- `PEM_ITEM_PKCS8_KEY (2)`：`PRIVATE KEY`
- `PEM_ITEM_PKCS1_KEY (3)`：`RSA PRIVATE KEY`
- `PEM_ITEM_SEC1_KEY (4)`：`EC PRIVATE KEY`
- `PEM_ITEM_SPKI_PUBKEY (5)`：`PUBLIC KEY`
- `PEM_ITEM_CRL (6)`：`X509 CRL`
- `PEM_ITEM_CSR (7)`：`CERTIFICATE REQUEST`
- `PEM_ITEM_UNKNOWN (8)`：其他合法 RFC 7468 标签

### 类型与函数

- `PemItem`：`kind: u8`、`der_len: usize`、`ok: bool`、`eof: bool`，方法 `isCert()` / `isPrivateKey()` / `makeEof()` / `makeErr()`。
- `PemBundleReader`：`new(input)`、`nextItem(der_out)`、`peekNextKind()`、`countItems()`、`countCerts()`、`remaining()`、`isEof()`、`reset()`。
- `readFirstPrivateKey(input: string, der_out: []u8): PemItem`：扫描并解码首个 PKCS#8 / PKCS#1 / SEC1 私钥。
- `classifyPemLabel(label: string): u8` / `isPrivateKeyKind(kind: u8): bool`。

## 许可证

MIT
