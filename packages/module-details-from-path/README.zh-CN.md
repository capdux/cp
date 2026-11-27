[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# module-details-from-path

> 版本：`1.0.4` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap node_modules and cap_modules path parser extracting package name, basedir, and subpath

## 安装与导入

```bash
cap add module-details-from-path
```

```cp
import { mdfpIsSep } from "module-details-from-path";
```

## 核心导出 API

- `export fn mdfpIsSep(ch: u8): bool;`
- `export fn mdfpMatchDir(s: string, pos: usize, dir: string): bool;`
- `export fn mdfpSkipSeps(s: string, start: usize): usize;`
- `export fn mdfpFindSep(s: string, start: usize): usize;`
- `export fn mdfpEmpty(): ModuleDetails;`
- `export fn parseModulePath(file_path: string): ModuleDetails;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`module-details-from-path@1.0.4`)（https://github.com/watson/module-details-from-path），许可证：MIT
