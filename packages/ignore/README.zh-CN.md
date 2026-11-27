[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ignore

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap .gitignore specification rule engine (clean-room CP implementation of npm ignore)

## 安装与导入

```bash
cap add ignore
```

```cp
import { igTrimRightSpaces } from "ignore";
```

## 核心导出 API

- `export fn igTrimRightSpaces(s: string): usize;`
- `export fn igHasInnerSlash(s: string, start: usize, end: usize): bool;`
- `export fn igRuleMatches(rule: IgnoreRule, rel_path: string, is_dir: bool, case_insensitive: bool): bool;`
- `export fn igMatchSingle(pat: &[]u8, anchored: bool, path: string, case_insensitive: bool): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ignore@1.0.0`)，许可证：MIT
