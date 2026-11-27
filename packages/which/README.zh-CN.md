[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# which

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap PATH executable candidate resolver for POSIX and Windows (clean-room CP implementation of npm which)

## 安装与导入

```bash
cap add which
```

```cp
import { whEqIgnoreCase } from "which";
```

## 核心导出 API

- `export fn whEqIgnoreCase(a: string, b: string): bool;`
- `export fn isExplicitPath(cmd: string): bool;`
- `export fn countPathDirs(path_env: string, delim: u8): usize;`
- `export fn pathDirAt(path_env: string, delim: u8, idx: usize, out: []u8): usize;`
- `export fn hasPathExt(cmd: string, pathext: string): bool;`
- `export fn formatCandidatePath(dir: string, cmd: string, ext: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`which@1.0.0`)，许可证：ISC
