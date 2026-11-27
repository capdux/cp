[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# color-name

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 148 W3C CSS Color 4 named colors lookup and exact RGB/HSL/HSV/HEX color space conversion engine for CP

## 安装与导入

```bash
cap add color-name
```

```cp
import { colorAt } from "color-name";
```

## 核心导出 API

- `export fn colorAt(index: usize): ColorEntry;`
- `export fn lookupColor(name: string): RgbColor;`
- `export fn lookupColorHex(name: string): i32;`
- `export fn reverseLookupColor(r: u8, g: u8, b: u8): string;`
- `export fn isNamedColor(name: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`color-name@1.0.0`)，许可证：MIT
