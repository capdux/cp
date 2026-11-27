[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/sparkline

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:sparkly` ([https://github.com/sindresorhus/sparkly](https://github.com/sindresorhus/sparkly))（原库名 `sparkly` → CP 包名 `@cap/sparkline`）

Zero-heap Unicode sparkline (`▁▂▃▄▅▆▇█`), fractional horizontal bar chart (`▏▎▍▌▋▊▉█`), bucket downsampling, and summary statistics renderer for CLI & observability dashboards.

Clean-room MIT implementation in pure CP (replaces npm `sparkly` / `asciichart` and Rust `sparklines`).

## Install

```bash
cap add @cap/sparkline
```

## Usage

```cp
import { render, renderAscii, renderBar, renderDownsampled, computeStats } from "@cap/sparkline";

const data: [f64; 8] = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0];
const spark = render(data[0..8]);      // "▁▂▃▄▅▆▇█"
const ascii = renderAscii(data[0..8]); // "._-:=+*#"
const bar   = renderBar(0.75, 8, false); // "██████  "
const stats = computeStats(data[0..8]);  // min=0, max=7, mean=3.5
```

## API

| Function | Description |
| --- | --- |
| `render(values)` | Render Unicode block sparkline (`▁▂▃▄▅▆▇█`) with auto min/max scaling |
| `renderAscii(values)` | Render ASCII-safe sparkline (`._-:=+*#`) |
| `renderWithOptions(values, opts)` | Render with custom `minBound`/`maxBound` clamping and ASCII toggle |
| `renderDownsampled(values, buckets, asciiOnly)` | Average-bucket downsample long series into `buckets` cells |
| `renderBar(ratio, width, fillAscii)` | Render horizontal progress/metric bar with 1/8th-block Unicode precision |
| `renderLabeledBar(label, ratio, width, fillAscii)` | Render `"label [bar]"` line |
| `computeStats(values)` | Compute `count`, `minVal`, `maxVal`, `sum`, `mean` in one pass |
| `quantizeLevel(value, lo, hi)` | Map scalar value into discrete level `0..7` |
