[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# indicatif

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 indicatif

## 安装与导入

```bash
cap add indicatif
```

```cp
import { indWriteByte } from "indicatif";
```

## 核心导出 API

- `export fn indWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn indWriteStr(out: []u8, pos: usize, s: &[]u8): usize;`
- `export fn indWriteU64(out: []u8, pos: usize, val: u64): usize;`
- `export fn percent(s: ProgressState): f64;`
- `export fn etaMs(s: ProgressState): u64;`
- `export fn bytesPerSec(s: ProgressState): f64;`
- `export fn renderBar(s: ProgressState, fill_ch: u8, head_ch: u8, empty_ch: u8, out: []u8): usize;`
- `export fn renderLine(s: ProgressState, prefix: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`indicatif@1.0.0`)，许可证：MIT
