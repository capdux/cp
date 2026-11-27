[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# minipass

> 版本：`1.0.0` | 许可证：`ISC` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation fixed-capacity ring-buffer byte stream with synchronous backpressure and piping for CP

## 安装与导入

```bash
cap add minipass
```

```cp
import { mpassMin } from "minipass";
```

## 核心导出 API

- `export fn mpassMin(a: usize, b: usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`minipass@1.0.0`)，许可证：ISC
