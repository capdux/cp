[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# oid-registry

> 版本：`1.0.0` | 许可证：`MIT OR Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation static X.509/PKCS/TLS/NIST/RFC OID registry with DER OID encoder and decoder (Rust oid-registry parity) for CP

## 安装与导入

```bash
cap add oid-registry
```

```cp
import { oidrEntryAt } from "oid-registry";
```

## 核心导出 API

- `export fn oidrEntryAt(idx: usize): OidEntry;`
- `export fn oidrStrEq(a: string, b: string): bool;`
- `export fn oidEntryCount(): usize;`
- `export fn lookupOid(dotted: string): OidEntry?;`
- `export fn lookupShortName(sn: string): OidEntry?;`
- `export fn oidrEncodeBase128(val: u64, out: []u8, pos: usize): usize?;`
- `export fn oidrWriteU64Dec(val: u64, out: []u8, pos: usize): usize?;`
- `export fn encodeOidDer(dotted: string, out: []u8): usize;`
- `export fn oidrDecodeBase128(der: &[]u8, start: usize): OidrDecodeStep;`
- `export fn decodeOidDer(der: &[]u8, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`oid-registry@1.0.0`)，许可证：MIT OR Apache-2.0
