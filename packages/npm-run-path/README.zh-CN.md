[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# npm-run-path

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap PATH builder that prepends ancestor node_modules/.bin (and cap_modules/.bin) directories for local binary execution (npm npm-run-path equivalent)

## 安装与导入

```bash
cap add npm-run-path
```

```cp
import { nrpPushBytes } from "npm-run-path";
```

## 核心导出 API

- `export fn nrpPushBytes(out: []u8, pos: usize, s: string): usize;`
- `export fn nrpPushChar(out: []u8, pos: usize, c: u8): usize;`
- `export fn nrpIsSep(c: u8, is_windows: bool): bool;`
- `export fn dirnameOf(path: string, is_windows: bool): string;`
- `export fn countAncestorDirs(cwd: string, is_windows: bool): usize;`
- `export fn buildRunPath(opts: RunPathOptions, out: []u8): usize;`
- `export fn pathKey(is_windows: bool): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`npm-run-path@1.0.0`)，许可证：MIT
