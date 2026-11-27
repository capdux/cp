[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# der

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation ITU-T X.690 ASN.1 DER encoder and strict decoder reusing const-oid for OID TLV handling (RustCrypto der parity) in pure CP.

## 安装与导入

```bash
cap add der
```

```cp
import { derEmptySlice } from "der";
```

## 核心导出 API

- `export fn derEmptySlice(): &[]u8;`
- `export fn derMinBytesUsize(v: usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`der@1.0.0`)，许可证：MIT OR Apache-2.0
