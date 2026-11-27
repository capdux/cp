[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ecdsa

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation ECDSA signature codec (RFC 3279 ASN.1 DER <-> fixed r||s for P-256/P-384, BIP-62 Low-S normalization, constant-time equality via der and subtle)

## 安装与导入

```bash
cap add ecdsa
```

```cp
import { example } from "ecdsa";
```

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`ecdsa@1.0.0`)，许可证：MIT
