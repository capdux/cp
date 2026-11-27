[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# half

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 half

## 安装与导入

```bash
cap add half
```

```cp
import { f32ToBits } from "half";
```

## 核心导出 API

- `export fn f32ToBits(x: f32): u32;`
- `export fn bitsToF32(b: u32): f32;`
- `export fn f32SliceToF16(src: &[]f32, dst: []u16): usize;`
- `export fn f16SliceToF32(src: &[]u16, dst: []f32): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`half@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`@petamoriken/float16@1.0.0`)，许可证：MIT
