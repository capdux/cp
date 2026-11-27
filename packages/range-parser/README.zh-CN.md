[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# range-parser

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 7233 HTTP Range header parser with suffix ranges and interval combining (clean-room port of npm range-parser)

## 安装与导入

```bash
cap add range-parser
```

```cp
import { parseRange } from "range-parser";
```

## 核心导出 API

- `export fn parseRange(size: u64, header: string, combine: bool): RangeResult;`
- `export fn rangeByteLength(r: ByteRange): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`range-parser@1.0.0`)，许可证：MIT
