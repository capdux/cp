[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# node-pid-controller

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap industrial PID controller with anti-windup clamping and derivative filtering in pure CP

## 安装与导入

```bash
cap add node-pid-controller
```

```cp
import { clampVal } from "node-pid-controller";
```

## 核心导出 API

- `export fn clampVal(x: f64, lo: f64, hi: f64): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`node-pid-controller@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`pid_control@1.0.0`)，许可证：MIT
