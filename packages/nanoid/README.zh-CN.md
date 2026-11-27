[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# nanoid

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap, URL-safe, bitmask-unbiased unique ID generator with custom alphabet and deterministic PRNG support (clean-room port of npm nanoid v5)

## 安装与导入

```bash
cap add nanoid
```

```cp
import { nidStepState } from "nanoid";
```

## 核心导出 API

- `export fn nidStepState(state: u64): u64;`
- `export fn nidBitmask(alphabet_len: usize): u8;`
- `export fn nidContainsByte(alphabet: string, ch: u8): bool;`
- `export fn nanoidFromBytes(bytes: []u8, size: usize, out: []u8): usize;`
- `export fn customAlphabetFromBytes(alphabet: string, bytes: []u8, size: usize, out: []u8): usize;`
- `export fn nanoidSeeded(seed: u64, size: usize, out: []u8): usize;`
- `export fn isValidNanoId(id: string, alphabet: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`nanoid@1.0.0`)，许可证：MIT
