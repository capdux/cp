[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# compressible

> 版本：`2.0.18` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation HTTP Content-Type compressibility checker

## 安装与导入

```bash
cap add compressible
```

```cp
import { cmprIsSpace } from "compressible";
```

## 核心导出 API

- `export fn cmprIsSpace(b: u8): bool;`
- `export fn cmprToLower(b: u8): u8;`
- `export fn cmprEqlIgnoreCase(a: string, b: string): bool;`
- `export fn cmprHasPrefixIgnoreCase(s: string, prefix: string): bool;`
- `export fn cmprHasSuffixIgnoreCase(s: string, suffix: string): bool;`
- `export fn cmprFindSlash(s: string): usize;`
- `export fn stripContentTypeParams(ct: string): string;`
- `export fn isCompressible(content_type: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`compressible@2.0.18`)（https://github.com/jshttp/compressible），许可证：MIT
