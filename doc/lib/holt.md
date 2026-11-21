# `cap:holt` — 纯 CP 零分配时间序列指数平滑与预测库

100% 纯 CP 零堆分配（Zero-Alloc）经典指数平滑预测标准库，覆盖：
- **SES**（Simple Exponential Smoothing，单指数平滑）；
- **Holt**（双参数线性趋势与 Gardner-McKenzie 阻尼趋势 `phi ∈ (0, 1]`）；
- **Holt-Winters**（三参数季节平滑，支持加法 `Additive` 与乘法 `Multiplicative` 季节性及可选阻尼趋势）；
- **预测评估指标**（MAE、RMSE、MAPE %、sMAPE %）。

## 导入方式

```cp
import {
    SeasonalityKind,
    SesResult,
    HoltConfig,
    HoltResult,
    HoltWintersConfig,
    HoltWintersResult,
    ForecastMetrics,
    sesForecast,
    holtForecast,
    holtWintersForecast,
    evaluateForecast,
} from "cap:holt";
```

## API 契约

### 1. 单指数平滑（SES）

- `pub struct SesResult { level: f64, sse: f64, rmse: f64 }`
- `pub fn sesForecast(series: []const f64, alpha: f64, horizon: usize, fitted_out: *[]f64, forecast_out: *[]f64): SesResult?`
  `alpha ∈ (0, 1]`；`fitted_out` 可传入空切片跳过拟合值写入，`forecast_out` 长度需 `>= horizon`。

### 2. Holt 线性/阻尼趋势平滑

- `pub struct HoltConfig { alpha: f64, beta: f64, phi: f64 }`（`HoltConfig.default()` 为 `{ alpha: 0.3, beta: 0.1, phi: 1.0 }`）
- `pub struct HoltResult { level: f64, trend: f64, sse: f64, rmse: f64 }`
- `pub fn holtForecast(series: []const f64, cfg: HoltConfig, horizon: usize, fitted_out: *[]f64, forecast_out: *[]f64): HoltResult?`
  要求 `series.len >= 2`，`alpha ∈ (0, 1]`、`beta ∈ [0, 1]`、`phi ∈ (0, 1]`。

### 3. Holt-Winters 季节指数平滑

- `pub enum SeasonalityKind { Additive, Multiplicative }`
- `pub struct HoltWintersConfig { alpha, beta, gamma, phi: f64, period: usize, kind: SeasonalityKind }`
- `pub struct HoltWintersResult { level: f64, trend: f64, sse: f64, rmse: f64 }`
- `pub fn holtWintersForecast(series: []const f64, cfg: HoltWintersConfig, horizon: usize, seasonal_buf: *[]f64, fitted_out: *[]f64, forecast_out: *[]f64): HoltWintersResult?`
  要求 `period >= 2`、`series.len >= 2 * period`、`seasonal_buf.*.len >= period`；乘法季节性要求全部样本严格 `> 0.0`。

### 4. 预测误差评估

- `pub struct ForecastMetrics { mae: f64, rmse: f64, mape: f64, smape: f64 }`（`mape`/`smape` 为百分比 `%`）
- `pub fn evaluateForecast(actual: []const f64, predicted: []const f64): ForecastMetrics?`

## 示例

```cp
import { HoltConfig, holtForecast } from "cap:holt";

fn demo() {
    const series: [6]f64 = [10.0, 12.5, 15.0, 17.5, 20.0, 22.5];
    let empty_buf: [0]f64 = [];
    let empty_fit: []f64 = empty_buf[0..];
    let fc_buf: [3]f64 = [0.0; 3];
    let fc_out: []f64 = fc_buf[0..];

    if (res := holtForecast(series[0..], HoltConfig.default(), 3, *empty_fit, *fc_out)) {
        * = res;
    }
}
```
