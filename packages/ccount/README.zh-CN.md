[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ccount

> 版本：`2.0.1` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation character/substring occurrence counter and longest consecutive streak finder

## 安装与导入

```bash
cap add ccount
```

```cp
import { ccountByte } from "ccount";
```

## 核心导出 API

- `export fn ccountByte(value: string, ch: u8): usize;`
- `export fn ccount(value: string, needle: string): usize;`
- `export fn longestStreakByte(value: string, ch: u8): usize;`
- `export fn longestStreak(value: string, needle: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ccount@2.0.1`)（https://github.com/wooorm/ccount），许可证：MIT
