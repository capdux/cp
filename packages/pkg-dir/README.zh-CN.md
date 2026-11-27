[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pkg-dir

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Node.js/CP package root directory finder locating nearest ancestor package.json or cap.json (npm pkg-dir equivalent, reuses @cap/find-up & @cap/npm-run-path)

## 安装与导入

```bash
cap add pkg-dir
```

```cp
import { pkdPush } from "pkg-dir";
```

## 核心导出 API

- `export fn pkdPush(out: []u8, pos: usize, s: string): usize;`
- `export fn packageDirectoryInList(cwd: string, existing_files: []string, out: []u8): usize?;`
- `export fn capPackageDirectoryInList(cwd: string, existing_files: []string, out: []u8): usize?;`
- `export fn packageDirectoryBounded(cwd: string, stop_at: string, existing_files: []string, out: []u8): usize?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`pkg-dir@1.0.0`)，许可证：MIT
