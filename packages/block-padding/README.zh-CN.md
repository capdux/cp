[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# block-padding

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation PKCS#7 (constant-time padding-oracle-resistant unpad), ISO 7816-4, ANSI X9.23, ZeroPadding, 128-bit BE/LE CTR counter increment, and XOR block primitives reusing @cap/subtle-ct in pure CP.

## 安装与导入

```bash
cap add block-padding
```

```cp
import { bpCopyBytes } from "block-padding";
```

## 核心导出 API

- `export fn bpCopyBytes(dst: []u8, src: &[]u8);`
- `export fn pkcs7Pad(input: &[]u8, block_size: usize, out: []u8): i64;`
- `export fn pkcs7UnpadCt(padded: &[]u8, block_size: usize): i64;`
- `export fn iso7816Pad(input: &[]u8, block_size: usize, out: []u8): i64;`
- `export fn iso7816Unpad(padded: &[]u8, block_size: usize): i64;`
- `export fn ansiX923Pad(input: &[]u8, block_size: usize, out: []u8): i64;`
- `export fn ansiX923Unpad(padded: &[]u8, block_size: usize): i64;`
- `export fn zeroPad(input: &[]u8, block_size: usize, out: []u8): i64;`
- `export fn zeroUnpad(padded: &[]u8, block_size: usize): i64;`
- `export fn ctrIncBe128(counter: []u8);`
- `export fn ctrIncLe128(counter: []u8);`
- `export fn xorBlock(dst: []u8, a: &[]u8, b: &[]u8): bool;`
- `export fn wipeBlock(buf: []u8);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`block-padding@1.0.0`)，许可证：MIT
