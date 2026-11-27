[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# aes

> 版本：`1.0.0` | 许可证：`MIT OR Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation pure CP FIPS-197 AES-128, AES-192, and AES-256 block cipher (RustCrypto aes parity)

## 安装与导入

```bash
cap add aes
```

```cp
import { aesXtime } from "aes";
```

## 核心导出 API

- `export fn aesXtime(x: u8): u8;`
- `export fn aesMul(a_in: u8, b_in: u8): u8;`
- `export fn aesSubBytes(state: []u8);`
- `export fn aesInvSubBytes(state: []u8);`
- `export fn aesShiftRows(s: []u8);`
- `export fn aesInvShiftRows(s: []u8);`
- `export fn aesMixColumns(s: []u8);`
- `export fn aesInvMixColumns(s: []u8);`
- `export fn aesAddRoundKey(s: []u8, rk: &[]u8, round: usize);`
- `export fn aesExpandKey(key: &[]u8, nk: usize, nr: usize, rk: []u8);`
- `export fn aesEncryptCore(block: [16]u8, rk: &[]u8, nr: usize): [16]u8;`
- `export fn aesDecryptCore(block: [16]u8, rk: &[]u8, nr: usize): [16]u8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`aes@1.0.0`)，许可证：MIT OR Apache-2.0
