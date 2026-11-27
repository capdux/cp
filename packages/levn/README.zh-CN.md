[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# levn

> 版本：`0.4.1` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Light Value Notation (levn) scalar, list, and object parser

## 安装与导入

```bash
cap add levn
```

```cp
import { lvnMakeInvalid } from "levn";
```

## 核心导出 API

- `export enum LevnKind`
- `export fn lvnMakeInvalid(): LevnValue;`
- `export fn lvnTryParseNumber(s: string): LevnValue;`
- `export fn parseLevnScalar(input: string): LevnValue;`
- `export fn lvnFindTopLevelChar(s: string, start: usize, target: u8): usize;`
- `export fn lvnUnquoteKey(s: string): string;`
- `export fn parseLevnList(input: string, out: []LevnValue): usize;`
- `export fn parseLevnObject(input: string, out: []LevnPair): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`levn@0.4.1`)（https://github.com/gkz/levn），许可证：MIT
