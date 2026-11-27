[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rfc6979

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation RFC 6979 deterministic ECDSA nonce generator (HMAC-SHA256 DRBG, RustCrypto rfc6979 parity)

## 安装与导入

```bash
cap add rfc6979
```

```cp
import { generateK } from "rfc6979";
```

## 核心导出 API

- `export fn generateK(priv_key_32: []u8, msg_hash_32: []u8, order_32: []u8, out_k32: []u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rfc6979@1.0.0`)，许可证：MIT
