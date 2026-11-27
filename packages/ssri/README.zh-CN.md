[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ssri

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation W3C Subresource Integrity (SRI) parser, validator, constant-time matcher, and formatter for CP

## 安装与导入

```bash
cap add ssri
```

```cp
import { algoName } from "ssri";
```

## 核心导出 API

- `export enum SriAlgo`
- `export fn algoName(algo: SriAlgo): string;`
- `export fn algoStrength(algo: SriAlgo): u8;`
- `export fn parseIntegrity(raw: string): SriIntegrity;`
- `export fn strongestHash(sri: SriIntegrity): SriHash?;`
- `export fn pickAlgorithm(sri: SriIntegrity, algo: SriAlgo): SriHash?;`
- `export fn matchesIntegrity(expected: SriIntegrity, actual: SriIntegrity): bool;`
- `export fn formatHash(algo: SriAlgo, base64_digest: string, out: []u8): string;`
- `export fn formatIntegrity(sri: SriIntegrity, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ssri@1.0.0`)，许可证：ISC
