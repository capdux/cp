# @cap/online-stats

Zero-allocation Welford streaming statistics, Chan parallel merge, OLS linear regression, and linear-interpolated percentiles for CP — clean-room MIT parity with Rust `average` / `criterion` statistical kernels.

纯 CP 零堆分配流式统计与基准指标引擎，支持抗大偏移灾难性抵消的 Welford 单遍方差/标准差、Chan 并行合并、最小二乘线性回归及 P50/P90/P95/P99/IQR 分位数计算。

## Install / 安装

```bash
cap add @cap/online-stats
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `RunningStats` | `init()`, `push(*_, x)`, `merge(*_, other)` | O(1) space Welford streaming stats with Chan parallel merge |
| `RunningStats` queries | `count`, `mean`, `varianceSample`, `variancePop`, `stddevSample`, `stddevPop`, `min`, `max`, `sum` | Numerically stable summary statistics |
| `LinearFit` | `struct { valid, slope, intercept, r_squared, pearson_r }` | OLS linear regression & Pearson correlation result |
| `linearRegression` | `(xs: []const f64, ys: []const f64) -> LinearFit` | Centered two-pass OLS regression |
| `percentile` / `median` / `iqr` | `(values: []const f64, ...) -> f64?` | Non-mutating stack-sorted linear-interpolated percentiles (up to 256 samples) |

## Usage / 使用示例

```cp
import { RunningStats, linearRegression, percentile } from "@cap/online-stats";

let st = RunningStats.init();
st.push(1000000004.0);
st.push(1000000007.0);
st.push(1000000013.0);
st.push(1000000016.0);
const v = st.varianceSample(); // 30.0 (no catastrophic cancellation!)
```
