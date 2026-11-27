[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/mbedtls

> 版本：`3.6.2` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

ARM Mbed TLS 3.6.2 core cryptography suite (SHA-256/512, AES-GCM, ChaCha20-Poly1305, Base64) for CP

## 安装与导入

```bash
cap add @cap/mbedtls
```

```cp
import { cp_mbedtls_version } from "@cap/mbedtls";
```

## 核心导出 API

- `export fn cp_mbedtls_version(buf: [*]u8!, cap: usize): usize;`
- `export fn cp_mbedtls_sha256(data: [*]&u8!?, len: usize, out32: [*]u8!): i32;`
- `export fn cp_mbedtls_aes_gcm_roundtrip_test(): i32;`
- `export fn versionInto(buf: []u8): !string;`
- `export fn sha256(data: &[]u8): ![32]u8;`
- `export fn aesGcmRoundtripTest(): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`mbedtls@3.6.2`)，许可证：Apache-2.0
