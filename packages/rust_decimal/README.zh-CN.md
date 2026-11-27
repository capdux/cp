[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rust_decimal

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 rust_decimal

## 安装与导入

```bash
cap add rust_decimal
```

```cp
import { absToU64 } from "rust_decimal";
```

## 核心导出 API

- `export fn absToU64(v: i64): u64;`
- `export fn clampScale(s: u8): u8;`
- `export fn pow10F64(s: u8): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rust_decimal@1.0.0`)，许可证：MIT
