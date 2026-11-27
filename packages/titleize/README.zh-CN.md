[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# titleize

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation string title-case formatter (npm titleize v3 parity) for CP

## 安装与导入

```bash
cap add titleize
```

```cp
import { titIsLower } from "titleize";
```

## 核心导出 API

- `export fn titIsLower(c: u8): bool;`
- `export fn titIsUpper(c: u8): bool;`
- `export fn titToUpper(c: u8): u8;`
- `export fn titToLower(c: u8): u8;`
- `export fn titIsWordChar(c: u8): bool;`
- `export fn upperCaseFirst(input: string, out: []u8): usize;`
- `export fn titleize(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`titleize@1.0.0`)，许可证：MIT
