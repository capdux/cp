[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# fletcher

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Fletcher-16, Fletcher-32, and Fletcher-64 position-sensitive checksums in pure CP

## 安装与导入

```bash
cap add fletcher
```

```cp
import { fletcher16 } from "fletcher";
```

## 核心导出 API

- `export fn fletcher16(data: string): u16;`
- `export fn fletcher16Bytes(data: []u8): u16;`
- `export fn fletcher32(data: string): u32;`
- `export fn fletcher32Bytes(data: []u8): u32;`
- `export fn fletcher64(data: string): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`fletcher@1.0.0`)，许可证：MIT
