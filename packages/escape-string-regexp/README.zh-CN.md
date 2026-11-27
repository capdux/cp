[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# escape-string-regexp

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Escape RegExp special characters in a string (clean-room CP implementation of npm escape-string-regexp)

## 安装与导入

```bash
cap add escape-string-regexp
```

```cp
import { esrIsSpecial } from "escape-string-regexp";
```

## 核心导出 API

- `export fn esrIsSpecial(c: u8): bool;`
- `export fn hasRegexpMeta(input: string): bool;`
- `export fn escapedRegexpLen(input: string): usize;`
- `export fn escapeStringRegexp(input: string, out: []u8): usize;`
- `export fn unescapeStringRegexp(escaped: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`escape-string-regexp@1.0.0`)，许可证：MIT
