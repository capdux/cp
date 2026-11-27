[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ecdsa-sig-formatter

> 版本：`1.0.0` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation ASN.1 DER <-> JOSE IEEE P1363 (r || s) ECDSA signature converter for ES256, ES384, and ES512 in pure CP

## 安装与导入

```bash
cap add ecdsa-sig-formatter
```

```cp
import { paramBytes } from "ecdsa-sig-formatter";
```

## 核心导出 API

- `export enum EcdsaAlg`
- `export fn paramBytes(alg: EcdsaAlg): usize;`
- `export fn esfParseDerInt(der: &[]u8, pos: usize, limit: usize, param_len: usize, dst: []u8): i64;`
- `export fn esfTrimLeadingZeros(src: &[]u8): usize;`
- `export fn derToJose(der: &[]u8, alg: EcdsaAlg, out: []u8): i64;`
- `export fn joseToDer(jose: &[]u8, alg: EcdsaAlg, out: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ecdsa-sig-formatter@1.0.0`)，许可证：Apache-2.0
