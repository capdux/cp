[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# sct

Zero-allocation RFC 6962 Certificate Transparency Signed Certificate Timestamp (SCT) parser and encoder (rustls/sct.rs parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:sct` ([https://github.com/rustls/sct.rs](https://github.com/rustls/sct.rs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `sct` |
| 版本 | `1.0.0` |
| 许可证 | `Apache-2.0 OR MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add sct
```

```cp
import {
    SctVersion,
    HashAlgorithm,
    SignatureAlgorithm,
    SignedCertificateTimestamp,
    sctInvalid,
    sctReadBeU16,
    sctReadBeU64,
    sctWriteBeU16,
} from "sct";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `SctVersion` | `V1, Unknown` |
| `enum` | `HashAlgorithm` | `None, Md5, Sha1, Sha224, Sha256, Sha384, Sha512, Unknown` |
| `enum` | `SignatureAlgorithm` | `Anonymous, Rsa, Dsa, Ecdsa, Ed25519, Ed448, Unknown` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SignedCertificateTimestamp` | `class` | 字段: `valid: bool, version: SctVersion, log_id: [32]u8, timestamp_ms: u64, extensions: &[]u8, hash_alg: HashAlgorithm, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sctInvalid` | `(): SignedCertificateTimestamp` | 调用 `sctInvalid` 并返回 `SignedCertificateTimestamp`。 |
| `sctReadBeU16` | `(b: &[]u8, off: usize): u16` | 调用 `sctReadBeU16` 并返回 `u16`。 |
| `sctReadBeU64` | `(b: &[]u8, off: usize): u64` | 调用 `sctReadBeU64` 并返回 `u64`。 |
| `sctWriteBeU16` | `(v: u16, out: []u8, off: usize)` | 就地执行 `sctWriteBeU16` 操作。 |
| `sctWriteBeU64` | `(v: u64, out: []u8, off: usize)` | 就地执行 `sctWriteBeU64` 操作。 |
| `sctDecodeHashAlg` | `(b: u8): HashAlgorithm` | 调用 `sctDecodeHashAlg` 并返回 `HashAlgorithm`。 |
| `sctEncodeHashAlg` | `(alg: HashAlgorithm): u8` | 调用 `sctEncodeHashAlg` 并返回 `u8`。 |
| `sctDecodeSigAlg` | `(b: u8): SignatureAlgorithm` | 调用 `sctDecodeSigAlg` 并返回 `SignatureAlgorithm`。 |
| `sctEncodeSigAlg` | `(alg: SignatureAlgorithm): u8` | 调用 `sctEncodeSigAlg` 并返回 `u8`。 |
| `parseSct` | `(data: &[]u8): SignedCertificateTimestamp` | 通过 `parseSct` 解析输入数据，返回 `SignedCertificateTimestamp`。 |
| `parseSctList` | `(tls_ext: &[]u8, out: []SignedCertificateTimestamp): i64` | 通过 `parseSctList` 解析输入数据，返回 `i64`。 |
| `encodeSct` | `(sct: SignedCertificateTimestamp, out: []u8): usize` | 通过 `encodeSct` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseSct } from "sct";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseSct(out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
