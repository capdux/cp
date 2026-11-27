[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# methods

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

HTTP methods supported by RFC 9110 / WebDAV with safe/idempotent/cacheable predicates (clean-room CP implementation of npm methods)

## 安装与导入

```bash
cap add methods
```

```cp
import { hmCmpLower } from "methods";
```

## 核心导出 API

- `export fn hmCmpLower(input: string, lower_target: string): i32;`
- `export fn findMethodIndex(method: string): i32;`
- `export fn isHttpMethod(method: string): bool;`
- `export fn toLowerMethod(method: string): string;`
- `export fn toUpperMethod(method: string): string;`
- `export fn methodAt(idx: usize): string;`
- `export fn isSafeMethod(method: string): bool;`
- `export fn isIdempotentMethod(method: string): bool;`
- `export fn isCacheableMethod(method: string): bool;`
- `export fn mayHaveRequestBody(method: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`methods@1.0.0`)，许可证：MIT
