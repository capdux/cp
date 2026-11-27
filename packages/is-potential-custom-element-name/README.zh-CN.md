[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# is-potential-custom-element-name

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation WHATWG HTML Custom Elements PotentialCustomElementName validator (clean-room MIT implementation of npm is-potential-custom-element-name, #407 / 132M wk)

## 安装与导入

```bash
cap add is-potential-custom-element-name
```

```cp
import { isPcenChar } from "is-potential-custom-element-name";
```

## 核心导出 API

- `export fn isPcenChar(cp: u32): bool;`
- `export fn isReservedCustomElementName(name: string): bool;`
- `export fn isPotentialCustomElementName(name: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`is-potential-custom-element-name@1.0.0`)，许可证：MIT
