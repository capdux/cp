# @cap/sct

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:sct` ([https://github.com/rustls/sct.rs](https://github.com/rustls/sct.rs))

Zero-allocation RFC 6962 Certificate Transparency Signed Certificate Timestamp (SCT) parser and encoder for CP — clean-room parity with `rustls/sct.rs` (362M+ downloads on crates.io, Apache-2.0 OR MIT).

纯 CP 零堆分配 RFC 6962 证书透明度（Certificate Transparency）SCT 线格式解析与序列化库，支持单条 SCT 严格边界解析、TLS 扩展 `SignedCertificateTimestampList` 列表解析与完整往返编码。

## Install / 安装

```bash
cap add @cap/sct
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `SctVersion` | `V1 = 0`, `Unknown = 255` | RFC 6962 SCT version enum |
| `HashAlgorithm` | `None`, `Md5`, `Sha1`, `Sha224`, `Sha256`, `Sha384`, `Sha512` | Digitally-signed hash algorithm |
| `SignatureAlgorithm` | `Anonymous`, `Rsa`, `Dsa`, `Ecdsa`, `Ed25519`, `Ed448` | Digitally-signed signature algorithm |
| `SignedCertificateTimestamp` | `valid`, `version`, `log_id`, `timestamp_ms`, `extensions`, `hash_alg`, `sig_alg`, `signature` | Parsed SCT view |
| `parseSct` | `(data: []const u8): SignedCertificateTimestamp` | Parse single RFC 6962 §3.2 SCT |
| `parseSctList` | `(tls_ext: []const u8, out: []SignedCertificateTimestamp): i64` | Parse RFC 6962 §3.3 TLS extension list |
| `encodeSct` | `(sct: SignedCertificateTimestamp, out: []u8): usize` | Serialize SCT back to wire bytes |

## Usage / 使用示例

```cp
import { parseSct, parseSctList, encodeSct } from "@cap/sct";

const sct = parseSct(wireBytes);
if (sct.valid) {
    let buf: [128]u8 = undefined;
    const n = encodeSct(sct, buf[0..128]);
}
```
