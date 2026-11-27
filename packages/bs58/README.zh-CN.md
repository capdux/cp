[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bs58

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Base58 encoder/decoder supporting Bitcoin, Ripple, and Flickr alphabets with leading-zero preservation (Rust bs58 / npm bs58 parity) for CP

## 安装与导入

```bash
cap add bs58
```

```cp
import { alphabetChars } from "bs58";
```

## 核心导出 API

- `export enum Base58Alphabet`
- `export fn alphabetChars(alpha: Base58Alphabet): string;`
- `export fn charIndex(alphabet: string, c: u8): i8;`
- `export fn isValidBase58(encoded: string, alpha: Base58Alphabet): bool;`
- `export fn encodeBase58With(input: string, alpha: Base58Alphabet, out: []u8): string?;`
- `export fn decodeBase58With(encoded: string, alpha: Base58Alphabet, out: []u8): string?;`
- `export fn encodeBase58(input: string, out: []u8): string?;`
- `export fn decodeBase58(encoded: string, out: []u8): string?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`bs58@1.0.0`)，许可证：MIT
