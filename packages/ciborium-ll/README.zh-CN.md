[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ciborium-ll

> 版本：`1.0.0` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 8949 low-level streaming CBOR encoder and decoder primitives in pure CP

## 安装与导入

```bash
cap add ciborium-ll
```

```cp
import { cblMaxI64AsU64 } from "ciborium-ll";
```

## 核心导出 API

- `export enum CborHeaderKind`
- `export fn cblMaxI64AsU64(): u64;`
- `export fn cblReadBe16(data: &[]u8, pos: usize): u16;`
- `export fn cblReadBe32(data: &[]u8, pos: usize): u32;`
- `export fn cblReadBe64(data: &[]u8, pos: usize): u64;`
- `export fn cblDecodeHalf(bits: u16): f64;`
- `export fn cblArgSize(val: u64): usize;`
- `export fn cblWriteHead(buf: []u8, pos: usize, major: u8, val: u64): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`ciborium-ll@1.0.0`)，许可证：Apache-2.0
