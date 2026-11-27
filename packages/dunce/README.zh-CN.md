[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# dunce

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 dunce

## 安装与导入

```bash
cap add dunce
```

```cp
import { isIllegalWinChar } from "dunce";
```

## 核心导出 API

- `export fn isIllegalWinChar(ch: u8): bool;`
- `export fn isWindowsReservedName(component: string): bool;`
- `export fn hasWindowsVerbatimPrefix(path: string): bool;`
- `export fn canStripVerbatimPrefix(path: string): bool;`
- `export fn simplified(path: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`dunce@1.0.0`)，许可证：MIT
