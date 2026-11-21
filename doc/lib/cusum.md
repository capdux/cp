# `cap:cusum` — 零堆分配在线变点检测与流式异常检测库

`cap:cusum` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的流式变点检测与统计过程控制（SPC）工具集，涵盖双边 Page CUSUM、无先验均值 Page-Hinkley 概念漂移检测、Roberts 时变控制限 EWMA 控制图、Welford 数值稳定流式 Z-Score 异常检测以及批量变点扫描。

## 核心特性

- **100% 零堆分配**：所有流式检测器均为轻量纯值类型状态机，`scanCusum` 使用调用方提供的输出切片。
- **双边 Page CUSUM（`CusumDetector` & `scanCusum`）**：均值突变检测工业标准，支持自定义允许漂移 `drift_k`、告警阈值 `threshold_h` 与 `reset_on_alarm` 自动重置开关。
- **Page-Hinkley 概念漂移检测（`PageHinkleyDetector`）**：无需已知基线均值，在线自适应追踪运行均值并双向检测上升/下降漂移。
- **EWMA 控制图（`EwmaControlChart`）**：内置精确 Roberts 时变上下控制限 $\mu_0 \pm L\sigma\sqrt{\frac{\lambda}{2-\lambda}(1-(1-\lambda)^{2t})}$，对小幅持续过程偏移高度敏感。
- **Welford 流式 Z-Score（`StreamingZScore`）**：采用 Welford 单遍方差递推彻底消除大基线偏移下的灾难性抵消，精准捕捉单点尖峰异常。

---

## 导出清单

### 枚举与结构体

- `ShiftDirection`：`None | Positive | Negative`
- `ChangePoint`：`{ index: usize, direction: ShiftDirection, stat: f64 }`
- `CusumDetector`：`init`, `reset`, `update`, `positiveStat`, `negativeStat`
- `PageHinkleyDetector`：`init`, `reset`, `update`
- `EwmaControlChart`：`init`, `reset`, `update`, `value`, `upperControlLimit`, `lowerControlLimit`
- `StreamingZScore`：`init`, `reset`, `update`, `currentMean`, `currentStd`

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `scanCusum(series, target_mean, drift_k, threshold_h, out_points): usize?` | 对序列执行双边 CUSUM 扫描并输出全部变点 |

---

## 使用示例

```typescript
import std from "std";
import {
    ShiftDirection,
    ChangePoint,
    CusumDetector,
    EwmaControlChart,
    scanCusum,
} from "cap:cusum";

fn cusumDemo() {
    let det = CusumDetector.init(0.0, 0.5, 4.0) ?? return;
    const d1 = det.update(0.1); // ShiftDirection.None
    const d2 = det.update(5.0); // ShiftDirection.Positive
    * = d1;
    * = d2;

    let chart = EwmaControlChart.init(0.0, 1.0, 0.2, 3.0) ?? return;
    * = chart.update(0.5);
    const ucl = chart.upperControlLimit();
    * = ucl;

    const series = [0.0, 0.1, -0.1, 3.0, 3.2, 3.1];
    let cps: [8]ChangePoint = std.mem.zeroes([8]ChangePoint);
    let cps_s: []ChangePoint = cps[0..];
    const found = scanCusum(series[0..], 0.0, 0.5, 4.0, &cps_s);
    * = found;
}
```
