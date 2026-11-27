[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# balanced-match

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation nested balanced delimiter matcher (npm balanced-match v3 parity) in pure CP

## 安装与导入

```bash
cap add balanced-match
```

```cp
import { balancedRange } from "balanced-match";
```

## 核心导出 API

- `export fn balancedRange(a: string, b: string, input: string): [usize, usize]?;`
- `export fn balanced(a: string, b: string, input: string): BalancedResult;`
- `export fn balancedAll(a: string, b: string, input: string, out: []BalancedResult): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`balanced-match@1.0.0`)，许可证：MIT
