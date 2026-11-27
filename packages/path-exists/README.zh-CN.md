[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# path-exists

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap path/file/directory existence checker with symlink awareness and batch filtering (npm path-exists equivalent, reuses locate-path)

## 安装与导入

```bash
cap add path-exists
```

```cp
import { pathExistsInEntries } from "path-exists";
```

## 核心导出 API

- `export fn pathExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool;`
- `export fn fileExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool;`
- `export fn dirExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool;`
- `export fn realPathExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool;`
- `export fn countExistingPaths(targets: []string, cwd: string, entries: []FsEntry, out_indices: []usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`path-exists@1.0.0`)，许可证：MIT
