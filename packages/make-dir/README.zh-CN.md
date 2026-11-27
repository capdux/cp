[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# make-dir

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap recursive directory creation planner with invalid-char validation, prefix chain generation, and octal permission mode (npm make-dir equivalent)

## 安装与导入

```bash
cap add make-dir
```

```cp
import { isValidDirPath } from "make-dir";
```

## 核心导出 API

- `export fn isValidDirPath(path: string, is_windows: bool): bool;`
- `export fn mkdStrEq(a: string, b: string): bool;`
- `export fn mkdExists(p: string, existing: []string): bool;`
- `export fn planMakeDir(target: string, existing_dirs: []string, opts: MakeDirOptions): MakeDirPlan;`
- `export fn applyUmask(mode: u32, umask: u32): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`make-dir@1.0.0`)，许可证：MIT
