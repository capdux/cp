[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# metaphone

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Lawrence Philips Metaphone English phonetic encoding algorithm in pure CP

## 安装与导入

```bash
cap add metaphone
```

```cp
import { isVowel } from "metaphone";
```

## 核心导出 API

- `export fn isVowel(b: u8): bool;`
- `export fn metaphone(word: string, max_len: usize, out: []u8): usize;`
- `export fn soundsLike(a: string, b: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`metaphone@1.0.0`)，许可证：MIT
