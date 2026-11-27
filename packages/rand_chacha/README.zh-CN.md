[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rand_chacha

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 8439 ChaCha8 / ChaCha12 / ChaCha20 block function and CSPRNG for CP.

## 安装与导入

```bash
cap add rand_chacha
```

```cp
import { chachaBlock } from "rand_chacha";
```

## 核心导出 API

- `export fn chachaBlock(key: [8]u32, counter: u32, nonce: [3]u32, rounds: u32, out: []u8);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rand_chacha@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`ppv-lite86@1.0.0`)，许可证：MIT
