[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# badge-maker

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Shields.io-style SVG status badge generator (flat, flat-square, plastic) with XML escaping and named colors

## 安装与导入

```bash
cap add badge-maker
```

```cp
import { defaultBadge } from "badge-maker";
```

## 核心导出 API

- `export enum BadgeStyle`
- `export fn defaultBadge(label: string, message: string, color: string): BadgeOptions;`
- `export fn resolveBadgeColor(name: string): string;`
- `export fn estimateTextWidth(s: string): usize;`
- `export fn renderBadge(opts: BadgeOptions, out: []u8): string;`
- `export fn makeBadge(label: string, message: string, color: string, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`badge-maker@1.0.0`)，许可证：CC0-1.0
