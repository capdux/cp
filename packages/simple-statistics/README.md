[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/stats-basic

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:simple-statistics` ([https://github.com/simple-statistics/simple-statistics](https://github.com/simple-statistics/simple-statistics))（原库名 `simple-statistics` → CP 包名 `@cap/stats-basic`）

Zero-heap Welford online statistics, Chan parallel merge, and descriptive statistics in pure CP (zero external dependencies).

## Features

- `OnlineStats`: numerically stable Welford `push`, Chan parallel `merge`, `variance`, `sampleVariance`, `stddev`, `sampleStddev`, `min_val`, `max_val`
- Slice stats: `sum`, `mean`, `variance`, `stddev`, `minSlice`, `maxSlice`, `covariance`, `pearsonR`, `percentileSorted`, `medianSorted`

## Usage

```cp
import { OnlineStats, pearsonR, medianSorted } from "@cap/stats-basic";

let st = OnlineStats.new();
st.push(2.0);
st.push(4.0);
st.push(4.0);
st.push(4.0);
const sd = st.stddev();
```
