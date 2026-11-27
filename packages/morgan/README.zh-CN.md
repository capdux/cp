[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# morgan

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap HTTP access log template formatter supporting Morgan tiny/dev/short/common/combined presets and custom tokens (clean-room port of morgan v1.10)

## 安装与导入

```bash
cap add morgan
```

```cp
import { mrgIsTokenChar } from "morgan";
```

## 核心导出 API

- `export fn mrgIsTokenChar(c: u8): bool;`
- `export fn mrgWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn mrgWriteOrDash(out: []u8, pos: usize, s: string): usize;`
- `export fn mrgWriteResponseTime(out: []u8, pos: usize, us: u64): usize;`
- `export fn formatMorganLog(format_str: string, ev: HttpLogEvent, out: []u8): usize;`
- `export fn formatPreset(preset: string, ev: HttpLogEvent, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`morgan@1.0.0`)，许可证：MIT
