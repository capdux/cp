[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# webpack-sources

> 版本：`3.2.3` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap source concatenation and range replacement engine

## 安装与导入

```bash
cap add webpack-sources
```

```cp
import { wsrcWriteStr } from "webpack-sources";
```

## 核心导出 API

- `export fn wsrcWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn wsrcEmptyEdit(): SourceEdit;`
- `export fn concatSources(parts: &[]string, sep: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`webpack-sources@3.2.3`)（https://github.com/webpack/webpack-sources），许可证：MIT
