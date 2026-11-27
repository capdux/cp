[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cli-truncate

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation ANSI-aware CLI string truncator (end/start/middle, CJK double-width, space boundary preference) in pure CP

## 安装与导入

```bash
cap add cli-truncate
```

```cp
import { cliTruncateInto } from "cli-truncate";
```

## 核心导出 API

- `export enum TruncatePosition`
- `export fn cliTruncateInto(text: string, columns: usize, opts: TruncateOptions, out: []u8): usize;`
- `export fn cliTruncateEndInto(text: string, columns: usize, out: []u8): usize;`
- `export fn cliTruncateStartInto(text: string, columns: usize, out: []u8): usize;`
- `export fn cliTruncateMiddleInto(text: string, columns: usize, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`cli-truncate@1.0.0`)，许可证：MIT
