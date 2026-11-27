[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# micromark-util-sanitize-uri

> 版本：`2.0.1` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation safe URI sanitizer and RFC 3986 percent-encoder blocking unsafe schemes

## 安装与导入

```bash
cap add micromark-util-sanitize-uri
```

```cp
import { isSafeUri } from "micromark-util-sanitize-uri";
```

## 核心导出 API

- `export fn isSafeUri(uri: string): bool;`
- `export fn sanitizeUri(uri: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`micromark-util-sanitize-uri@2.0.1`)（https://github.com/micromark/micromark），许可证：MIT
