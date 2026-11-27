[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bytesize

> 版本：`1.0.0` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 bytesize

## 安装与导入

```bash
cap add bytesize
```

```cp
import { bszIsSpace } from "bytesize";
```

## 核心导出 API

- `export fn bszIsSpace(b: u8): bool;`
- `export fn bszIsDigit(b: u8): bool;`
- `export fn bszToLower(b: u8): u8;`
- `export fn bszWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn bszWriteSlice(out: []u8, pos: usize, s: string): usize;`
- `export fn bszWriteU64(out: []u8, pos: usize, val: u64): usize;`
- `export fn bszUnitMultiplier(s: string): u64?;`
- `export fn bszFormatScaled(bytes: u64, unit: u64, suffix: string, out: []u8): usize;`
- `export fn formatSi(bytes: u64, out: []u8): usize;`
- `export fn formatIec(bytes: u64, out: []u8): usize;`
- `export fn parseByteSize(input: string): u64?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`bytesize@1.0.0`)，许可证：Apache-2.0
