[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# anymatch

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 anymatch

## 安装与导入

```bash
cap add anymatch
```

```cp
import { bytesEq } from "anymatch";
```

## 核心导出 API

- `export fn bytesEq(a: &[]u8, b: &[]u8): bool;`
- `export fn startsWithDirPrefix(path: &[]u8, prefix: &[]u8): bool;`
- `export fn globMatchAt(pat: &[]u8, pi_in: usize, txt: &[]u8, ti_in: usize): bool;`
- `export fn matchPattern(pattern: string, test_path: string): bool;`
- `export fn anymatchIndex(patterns: &[]string, test_path: string): i64;`
- `export fn anymatch(patterns: &[]string, test_path: string): bool;`
- `export fn countGlobPatterns(patterns: &[]string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`anymatch@1.0.0`)，许可证：MIT
