[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# sketches-ddsketch

> 版本：`0.3.0` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap DDSketch relative-accuracy quantile sketch for low-memory latency statistics

## 安装与导入

```bash
cap add sketches-ddsketch
```

```cp
import { ddskZeroBuckets } from "sketches-ddsketch";
```

## 核心导出 API

- `export fn ddskZeroBuckets(): [DDSK_BUCKETS]u64;`
- `export fn ddskClampAlpha(alpha: f64): f64;`
- `export fn ddskAddKeyCount(sk: &mut DDSketch, key: i32, cnt: u64);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`sketches-ddsketch@0.3.0`)（https://github.com/DataDog/sketches-ddsketch-rust），许可证：Apache-2.0
