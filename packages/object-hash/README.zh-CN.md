[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# object-hash

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation deterministic order-independent structured object hasher using FNV-1a 64-bit (npm object-hash parity) in pure CP

## 安装与导入

```bash
cap add object-hash
```

```cp
import { ohshFnvOffset } from "object-hash";
```

## 核心导出 API

- `export fn ohshFnvOffset(): u64;`
- `export fn ohshFnvPrime(): u64;`
- `export fn ohshFeedByte(h: u64, b: u8): u64;`
- `export fn ohshFeedBytes(h: u64, data: &[]u8): u64;`
- `export fn ohshCmpStr(a: string, b: string): i32;`
- `export fn ohshFieldLess(a: HashField, b: HashField): bool;`
- `export fn ohshHexDigit(nibble: u8): u8;`
- `export fn hashBytesFnv64(data: &[]u8): u64;`
- `export fn hashStringFnv64(s: string): u64;`
- `export fn hashFieldsFnv64(fields: &[]HashField): u64;`
- `export fn hashFieldsHex(fields: &[]HashField, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`object-hash@1.0.0`)，许可证：MIT
