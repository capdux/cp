[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# accept-header

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 7231 HTTP Accept header parser and media-type negotiator with quality-factor & specificity ranking (clean-room port of Rust accept-header)

## 安装与导入

```bash
cap add accept-header
```

```cp
import { acphFindSlash } from "accept-header";
```

## 核心导出 API

- `export fn acphFindSlash(s: string): usize;`
- `export fn acphIsStar(s: string): bool;`
- `export fn acphMediaSpecificity(range: string): u8;`
- `export fn acphMatchesRange(range: string, mime: string): bool;`
- `export fn acphEmptyEntry(): AcceptEntry;`
- `export fn acphEmptySpec(): QualitySpec;`
- `export fn negotiateMediaType(header: string, available: []string): string?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`accept-header@1.0.0`)，许可证：MIT
