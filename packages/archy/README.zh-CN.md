[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# archy

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Unicode & ASCII dependency/directory tree renderer (archy / treeify / Rust ptree parity) for CP

## 安装与导入

```bash
cap add archy
```

```cp
import { renderTreeWith } from "archy";
```

## 核心导出 API

- `export enum TreeStyle`
- `export fn renderTreeWith(tree: &mut TreeBuilder, style: TreeStyle, out: []u8): string;`
- `export fn renderTree(tree: &mut TreeBuilder, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`archy@1.0.0`)，许可证：MIT
