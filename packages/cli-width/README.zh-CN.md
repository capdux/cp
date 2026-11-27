[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cli-width

> 版本：`1.0.0` | 许可证：`ISC` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation terminal column width resolver with TTY, COLUMNS env, default, and min/max clamping (npm cli-width parity) in pure CP

## 安装与导入

```bash
cap add cli-width
```

```cp
import { cliwIsSpace } from "cli-width";
```

## 核心导出 API

- `export fn cliwIsSpace(c: u8): bool;`
- `export fn defaultCliWidth(): u32;`
- `export fn parseColumnsEnv(env_val: string): u32?;`
- `export fn resolveCliWidth(tty_cols: u32, env_columns: string, opts: CliWidthOptions): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`cli-width@1.0.0`)，许可证：ISC
