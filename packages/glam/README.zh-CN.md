[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# glam

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap 3D math library with Vec3, unit Quat, and column-major Mat4 (Rust glam parity, pure CP with built-in sqrt/sin/cos).

## 安装与导入

```bash
cap add glam
```

```cp
import { absF64 } from "glam";
```

## 核心导出 API

- `export fn absF64(x: f64): f64;`
- `export fn sqrtF64(x: f64): f64;`
- `export fn sinF64(angle: f64): f64;`
- `export fn cosF64(angle: f64): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`glam@1.0.0`)，许可证：MIT
