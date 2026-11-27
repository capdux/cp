[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# password-hash

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation pure CP PHC string format parser, serializer, and constant-time comparator (RustCrypto password-hash parity)

## 安装与导入

```bash
cap add password-hash
```

```cp
import { phchEmptyHash } from "password-hash";
```

## 核心导出 API

- `export fn phchEmptyHash(): PhcHash;`
- `export fn phchIsLowerAlnumHyphen(c: u8): bool;`
- `export fn phchIsParamValueChar(c: u8): bool;`
- `export fn phchIsB64Char(c: u8): bool;`
- `export fn phchIsValidAlgo(s: string): bool;`
- `export fn phchIsValidParamKey(s: string): bool;`
- `export fn phchIsValidParamValue(s: string): bool;`
- `export fn phchIsValidSalt(s: string): bool;`
- `export fn phchIsValidHash(s: string): bool;`
- `export fn phchContainsByte(s: string, target: u8): bool;`
- `export fn phchParseU32(s: string): i64;`
- `export fn phchStrEq(a: string, b: string): bool;`
- `export fn phchAppendBytes(out: []u8, pos: usize, src: string): usize;`
- `export fn phchAppendU32(out: []u8, pos: usize, val: u32): usize;`
- `export fn parsePhc(phc_str: string): PhcHash;`
- `export fn formatPhc(h: PhcHash, out: []u8): usize;`
- `export fn getPhcParam(h: PhcHash, key: string): string?;`
- `export fn ctEqual(a: string, b: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`password-hash@1.0.0`)，许可证：MIT
