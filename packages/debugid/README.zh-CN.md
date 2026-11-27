[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# debugid

> 版本：`0.8.0` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Sentry/Breakpad/PDB DebugId (128-bit UUID + appendix age) parser and formatter

## 安装与导入

```bash
cap add debugid
```

```cp
import { parseDebugId } from "debugid";
```

## 核心导出 API

- `export fn parseDebugId(s: string): DebugId;`
- `export fn formatBreakpad(id: DebugId, out: []u8): usize;`
- `export fn formatCanonical(id: DebugId, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`debugid@0.8.0`)（https://github.com/getsentry/rust-debugid），许可证：Apache-2.0
