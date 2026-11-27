[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# has-flag

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Check if CLI argv contains a specific flag (clean-room CP implementation of npm has-flag)

## 安装与导入

```bash
cap add has-flag
```

```cp
import { hfEq } from "has-flag";
```

## 核心导出 API

- `export fn hfEq(a: string, b: string): bool;`
- `export fn hfStartsWith(s: string, prefix: string): bool;`
- `export fn hfMatchesFlag(arg: string, flag: string, prefix_len: usize): bool;`
- `export fn hfComputePrefixLen(flag: string): usize;`
- `export fn findFlagIndex(flag: string, argv: &[]string): i64;`
- `export fn hasFlag(flag: string, argv: &[]string): bool;`
- `export fn getFlagValue(flag: string, argv: &[]string, out: []u8): i64;`
- `export fn isFlagEnabled(flag: string, argv: &[]string, default_val: bool): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`has-flag@1.0.0`)，许可证：MIT
