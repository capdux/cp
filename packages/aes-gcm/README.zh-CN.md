[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# aes-gcm

> 版本：`1.0.0` | 许可证：`Apache-2.0 OR MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation NIST SP 800-38D AES-128-GCM and AES-256-GCM AEAD authenticated encryption composing @cap/aes and @cap/ghash-gf128 (RustCrypto aes-gcm parity) for CP

## 安装与导入

```bash
cap add aes-gcm
```

```cp
import { agcmZeroBlock } from "aes-gcm";
```

## 核心导出 API

- `export fn agcmZeroBlock(): [16]u8;`
- `export fn agcmMakeCounterBlock(nonce: [12]u8, counter: u32): [16]u8;`
- `export fn agcmCtEq16(a: [16]u8, b: [16]u8): bool;`
- `export fn agcmComputeTag(h: [16]u8, e_j0: [16]u8, aad: &[]u8, ct: &[]u8): [16]u8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`aes-gcm@1.0.0`)，许可证：Apache-2.0 OR MIT
