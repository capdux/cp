[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# salsa20

> 版本：`0.10.2` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Salsa20/20 stream cipher (256-bit key, 64-bit nonce) verified against official Bernstein/eSTREAM vectors

## 安装与导入

```bash
cap add salsa20
```

```cp
import { salsa20Block } from "salsa20";
```

## 核心导出 API

- `export fn salsa20Block(key: &[]u8, nonce: &[]u8, counter: u64, out: []u8);`
- `export fn salsa20Apply(key: &[]u8, nonce: &[]u8, initial_counter: u64, input: &[]u8, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`salsa20@0.10.2`)（https://github.com/RustCrypto/stream-ciphers），许可证：MIT
