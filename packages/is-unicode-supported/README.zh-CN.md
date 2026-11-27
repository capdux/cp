[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# is-unicode-supported

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation pure-logic terminal Unicode support detector (npm is-unicode-supported v2 parity) for CP

## 安装与导入

```bash
cap add is-unicode-supported
```

```cp
import { iusStrEq } from "is-unicode-supported";
```

## 核心导出 API

- `export fn iusStrEq(a: string, b: string): bool;`
- `export fn isUnicodeSupportedTerm(term: string): bool;`
- `export fn isUnicodeSupportedTermProgram(term_program: string): bool;`
- `export fn isUnicodeSupported(env: EnvInfo): bool;`
- `export fn isUnicodeSupportedPlatform(is_windows: bool, term: string): bool;`
- `export fn selectFallback(env: EnvInfo, unicode_val: string, ascii_val: string): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`is-unicode-supported@1.0.0`)，许可证：MIT
