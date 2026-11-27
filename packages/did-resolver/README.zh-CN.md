[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# did-resolver

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap W3C DID Core 1.0 Decentralized Identifier & DID URL parser, strict character validator, and formatter.

## 安装与导入

```bash
cap add did-resolver
```

```cp
import { isValidMethod } from "did-resolver";
```

## 核心导出 API

- `export fn isValidMethod(method: string): bool;`
- `export fn isValidMethodId(id: string): bool;`
- `export fn parseDid(input: string): DidUrl;`
- `export fn appendSlice(out: []u8, pos: usize, s: string): i64;`
- `export fn formatDid(method: string, id: string, fragment: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`did-resolver@1.0.0`)，许可证：Apache-2.0
- **上游生态**：`crates.io` (`ssi-dids@1.0.0`)，许可证：Apache-2.0
