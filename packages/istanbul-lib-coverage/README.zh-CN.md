[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# istanbul-lib-coverage

> 版本：`3.2.2` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap code coverage metrics, summary aggregation, and LCOV codec

## 安装与导入

```bash
cap add istanbul-lib-coverage
```

```cp
import { icovWriteByte } from "istanbul-lib-coverage";
```

## 核心导出 API

- `export fn icovWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn icovParseU32(s: string): u32;`
- `export fn formatLcovRecord(file_path: string, hits: &[]LineHit, out: []u8): usize;`
- `export fn parseLcovSummary(lcov_text: string): CoverageMetric;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`istanbul-lib-coverage@3.2.2`)（https://github.com/istanbuljs/istanbuljs），许可证：BSD-3-Clause
