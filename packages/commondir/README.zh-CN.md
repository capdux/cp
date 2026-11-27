[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# commondir

> 版本：`1.0.1` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap computation of the deepest common parent directory across a list of file paths

## 安装与导入

```bash
cap add commondir
```

```cp
import { commonDirTwo } from "commondir";
```

## 核心导出 API

- `export fn commonDirTwo(a: string, b: string): string;`
- `export fn commonDir(paths: &[]string): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`commondir@1.0.1`)（https://github.com/substack/node-commondir），许可证：MIT
