[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pid

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation industrial PID feedback controller with term limits, anti-windup, and derivative-on-measurement in pure CP

## 安装与导入

```bash
cap add pid
```

```cp
import { clampRange } from "pid";
```

## 核心导出 API

- `export fn clampRange(v: f64, lo: f64, hi: f64): f64;`
- `export fn clampSymmetric(v: f64, lim: f64): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`pid@1.0.0`)，许可证：MIT
