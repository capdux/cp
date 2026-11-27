[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# sparkly

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Unicode sparkline (▁▂▃▄▅▆▇█), horizontal bar chart, and summary statistics renderer for CLI & monitoring

## 安装与导入

```bash
cap add sparkly
```

```cp
import { defaultOptions } from "sparkly";
```

## 核心导出 API

- `export fn defaultOptions(): SparkOptions;`
- `export fn computeStats(values: &[]f64): SeriesStats;`
- `export fn appendBlock(res: &mut SparkResult, level: u8, asciiOnly: bool);`
- `export fn appendByte(res: &mut SparkResult, b: u8);`
- `export fn appendBytes(res: &mut SparkResult, s: string);`
- `export fn quantizeLevel(value: f64, lo: f64, hi: f64): u8;`
- `export fn renderWithOptions(values: &[]f64, opts: SparkOptions): SparkResult;`
- `export fn render(values: &[]f64): SparkResult;`
- `export fn renderAscii(values: &[]f64): SparkResult;`
- `export fn renderBar(ratio: f64, width: usize, fillAscii: bool): SparkResult;`
- `export fn renderDownsampled(values: &[]f64, targetBuckets: usize, asciiOnly: bool): SparkResult;`
- `export fn renderLabeledBar(label: string, ratio: f64, barWidth: usize, fillAscii: bool): SparkResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`sparkly@1.0.0`)，许可证：MIT
