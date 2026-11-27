[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bwt

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Burrows-Wheeler Transform (BWT) and Move-To-Front (MTF) reversible compression preprocessing in pure CP

## 安装与导入

```bash
cap add bwt
```

```cp
import { cmpRotations } from "bwt";
```

## 核心导出 API

- `export fn cmpRotations(src: &[]u8, a: usize, b: usize): i32;`
- `export fn bwtForward(src: &[]u8, dst: []u8): BwtResult;`
- `export fn bwtInverse(src: &[]u8, primary_index: usize, dst: []u8): bool;`
- `export fn mtfEncode(src: &[]u8, dst: []u8): bool;`
- `export fn mtfDecode(src: &[]u8, dst: []u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`bwt@1.0.0`)，许可证：MIT
