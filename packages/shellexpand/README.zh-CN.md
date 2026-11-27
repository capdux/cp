[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# shellexpand

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation shell-like tilde (~) and environment variable ($VAR, ${VAR}, ${VAR:-default}) expansion (clean-room MIT implementation of crates.io shellexpand)

## 安装与导入

```bash
cap add shellexpand
```

```cp
import { tildeWithHome } from "shellexpand";
```

## 核心导出 API

- `export fn tildeWithHome(input: string, home_dir: string, out: []u8): usize;`
- `export fn envExpand(input: string, vars: &[]EnvEntry, keep_unknown: bool, out: []u8): usize;`
- `export fn fullExpand(input: string, home_dir: string, vars: &[]EnvEntry, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`shellexpand@1.0.0`)，许可证：MIT OR Apache-2.0
