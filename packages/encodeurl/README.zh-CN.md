[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# encodeurl

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap idempotent URL encoder that percent-encodes unsafe characters without double-encoding existing %XX sequences (clean-room port of npm encodeurl)

## 安装与导入

```bash
cap add encodeurl
```

```cp
import { euIsHex } from "encodeurl";
```

## 核心导出 API

- `export fn euIsHex(b: u8): bool;`
- `export fn euNeedsEncode(b: u8): bool;`
- `export fn encodeUrl(url: string, out: []u8): usize;`
- `export fn isEncodedUrl(url: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`encodeurl@1.0.0`)，许可证：MIT
