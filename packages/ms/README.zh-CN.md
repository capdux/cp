[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ms

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap millisecond conversion utility (clean-room CP implementation of npm ms)

## 安装与导入

```bash
cap add ms
```

```cp
import { msUnitMultiplier } from "ms";
```

## 核心导出 API

- `export fn msUnitMultiplier(s: string, start: usize, end: usize): f64;`
- `export fn parseMs(input: string): MsParseResult;`
- `export fn msRoundDiv(ms: i64, unit: i64): i64;`
- `export fn msFormatUnit(val: i64, short_unit: string, long_singular: string, long_plural: string, long_fmt: bool, out: []u8): usize;`
- `export fn formatMs(ms: i64, long_fmt: bool, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ms@1.0.0`)，许可证：MIT
