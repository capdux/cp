[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# iso8601-duration

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation ISO 8601 duration parser, canonical formatter, and millisecond converter in pure CP

## 安装与导入

```bash
cap add iso8601-duration
```

```cp
import { invalidDuration } from "iso8601-duration";
```

## 核心导出 API

- `export fn invalidDuration(): IsoDuration;`
- `export fn parseIsoDuration(src: &[]u8): IsoDuration;`
- `export fn pushByte(dst: []u8, pos: usize, b: u8): i64;`
- `export fn pushU32(dst: []u8, pos: usize, val: u32): i64;`
- `export fn pushComp(dst: []u8, pos: usize, val: u32, unit: u8): i64;`
- `export fn formatIsoDuration(d: IsoDuration, dst: []u8): i64;`
- `export fn toApproxMillis(d: IsoDuration): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`iso8601-duration@1.0.0`)，许可证：MIT
