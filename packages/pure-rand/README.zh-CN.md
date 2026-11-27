[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pure-rand

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation pure functional immutable PRNG (Xoroshiro128Plus, SplitMix64 seeding, jump, and unbiased uniformIntDistribution) (clean-room MIT implementation of npm pure-rand, #443 / 124M wk)

## 安装与导入

```bash
cap add pure-rand
```

```cp
import { uniformIntDistribution } from "pure-rand";
```

## 核心导出 API

- `export fn uniformIntDistribution(min: i64, max: i64, rng: Xoroshiro128Plus): NextIntRangeResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`pure-rand@1.0.0`)，许可证：MIT
