[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# sct

> 版本：`1.0.0` | 许可证：`Apache-2.0 OR MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 6962 Certificate Transparency Signed Certificate Timestamp (SCT) parser and encoder (rustls/sct.rs parity) for CP

## 安装与导入

```bash
cap add sct
```

```cp
import { sctInvalid } from "sct";
```

## 核心导出 API

- `export enum SctVersion`
- `export enum HashAlgorithm`
- `export enum SignatureAlgorithm`
- `export fn sctInvalid(): SignedCertificateTimestamp;`
- `export fn sctReadBeU16(b: &[]u8, off: usize): u16;`
- `export fn sctReadBeU64(b: &[]u8, off: usize): u64;`
- `export fn sctWriteBeU16(v: u16, out: []u8, off: usize);`
- `export fn sctWriteBeU64(v: u64, out: []u8, off: usize);`
- `export fn sctDecodeHashAlg(b: u8): HashAlgorithm;`
- `export fn sctEncodeHashAlg(alg: HashAlgorithm): u8;`
- `export fn sctDecodeSigAlg(b: u8): SignatureAlgorithm;`
- `export fn sctEncodeSigAlg(alg: SignatureAlgorithm): u8;`
- `export fn parseSct(data: &[]u8): SignedCertificateTimestamp;`
- `export fn parseSctList(tls_ext: &[]u8, out: []SignedCertificateTimestamp): i64;`
- `export fn encodeSct(sct: SignedCertificateTimestamp, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`sct@1.0.0`)，许可证：Apache-2.0 OR MIT
