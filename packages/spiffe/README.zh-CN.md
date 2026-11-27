[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# spiffe

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation CNCF SPIFFE ID (spiffe://<trust-domain>/<path>) parser, strict security validator, and formatter for CP.

## 安装与导入

```bash
cap add spiffe
```

```cp
import { isValidTrustDomain } from "spiffe";
```

## 核心导出 API

- `export fn isValidTrustDomain(td: string): bool;`
- `export fn isValidPath(path: string): bool;`
- `export fn parseSpiffeId(uri: string): SpiffeId;`
- `export fn belongsToTrustDomain(id: SpiffeId, trust_domain: string): bool;`
- `export fn formatSpiffeId(trust_domain: string, path: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`spiffe@1.0.0`)，许可证：Apache-2.0
