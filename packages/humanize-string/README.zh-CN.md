[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# humanize-string

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation camelCase/snake_case/kebab-case to humanized sentence converter (npm humanize-string v3 parity) composing decamelize for CP

## 安装与导入

```bash
cap add humanize-string
```

```cp
import { hstrIsSep } from "humanize-string";
```

## 核心导出 API

- `export fn hstrIsSep(c: u8): bool;`
- `export fn hstrToUpper(c: u8): u8;`
- `export fn humanizeString(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`humanize-string@1.0.0`)，许可证：MIT
