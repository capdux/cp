[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# kalmanjs

面向 CP 语言的零堆分配一维标量卡尔曼滤波器，用于传感器去噪、信号平滑与线性状态预测。

对齐 npm 源包 [`kalmanjs`](https://github.com/wouterbulten/kalmanjs)（MIT）。

## 安装与导入

```bash
cap add kalmanjs
```

```cp
import { Kalman1D, KalmanFilter } from "kalmanjs";
```

## 核心 API

### `KalmanFilter`（对齐 npm `kalmanjs` 命名）

| 成员 | 签名 | 说明 |
| --- | --- | --- |
| 字段 | `R, Q, A, B, C, x, cov: f64`, `initialized: bool` | 过程噪声 `R`、测量噪声 `Q`、状态/控制/观测系数 `A/B/C`、状态估计 `x`、协方差 `cov`。 |
| `KalmanFilter.default` | `(): KalmanFilter` | 默认滤波器（`R=1.0, Q=1.0, A=1.0, B=0.0, C=1.0`）。 |
| `KalmanFilter.new` | `(R: f64, Q: f64): KalmanFilter` | 指定过程噪声 `R` 与测量噪声 `Q` 构造滤波器。 |
| `KalmanFilter.withParams` | `(R: f64, Q: f64, A: f64, B: f64, C: f64): KalmanFilter` | 指定完整线性模型参数。 |
| `filter` | `(*self, z: f64): f64` | 滤波单次测量值 `z`（控制输入 `u = 0.0`），返回更新后的估计值。 |
| `filterWithControl` | `(*self, z: f64, u: f64): f64` | 带控制输入 `u` 的卡尔曼滤波更新。 |
| `predict` | `(u: f64): f64` | 预测下一状态，不修改内部状态。 |
| `uncertainty` / `variance` | `(): f64` | 返回当前协方差（不确定度 `cov`）。 |
| `lastMeasurement` | `(): f64` | 返回最新状态估计值 `x`。 |
| `setProcessNoise` / `setMeasurementNoise` | `(*self, noise: f64)` | 动态调整过程噪声或测量噪声。 |
| `reset` | `(*self)` | 重置 `initialized`、`x`、`cov` 为未初始化状态。 |

### `Kalman1D`（标准控制理论命名）

| 成员 | 签名 | 说明 |
| --- | --- | --- |
| 字段 | `q, r, a, b, c, x, cov: f64`, `initialized: bool` | 标准符号：`q` 为过程噪声，`r` 为测量噪声。 |
| `Kalman1D.new` | `(process_noise: f64, measurement_noise: f64): Kalman1D` | 以 `(q, r)` 构造，默认 `a=1, b=0, c=1`。 |
| `Kalman1D.withModel` | `(q: f64, r: f64, a: f64, b: f64, c: f64): Kalman1D` | 完整线性模型构造器。 |
| `filter` | `(*self, z: f64, u: f64): f64` | 带控制输入 `u` 的滤波更新。 |
| `filterOnly` | `(*self, z: f64): f64` | 仅传入测量值 `z`（`u = 0.0`）的滤波更新。 |
| `predict` / `uncertainty` / `variance` / `lastMeasurement` | `(u: f64): f64` / `(): f64` | 只读状态查询方法。 |
| `setProcessNoise` / `setMeasurementNoise` / `reset` | `(*self, ...)` | 可变参数调整与状态重置。 |

## 使用示例

### 1. 传感器噪声平滑（`KalmanFilter`）

```cp
import { print } from "cap:io";
import { KalmanFilter } from "kalmanjs";

pub fn main() {
    let kf = KalmanFilter.new(0.01, 3.0);
    let readings: [5]f64 = [20.1, 20.8, 19.7, 20.3, 20.0];
    let i: usize = 0;
    while (i < 5) {
        let est = kf.filter(readings[i]);
        print("step {}: est={}\n", i, est);
        i += 1;
    }
}
```

### 2. 带控制输入的线性系统跟踪（`Kalman1D`）

```cp
import { print } from "cap:io";
import { Kalman1D } from "kalmanjs";

pub fn main() {
    let kf = Kalman1D.withModel(0.01, 1.0, 1.0, 1.0, 1.0);
    let _ = kf.filter(10.0, 1.0);
    let est = kf.filter(11.4, 1.0);
    print("est={}, next_pred={}, cov={}\n", est, kf.predict(1.0), kf.uncertainty());
}
```

## 与源包差异与零拷贝安全亮点

- **纯栈值类型**：`KalmanFilter` 与 `Kalman1D` 为紧凑栈结构体（约 64 字节），零堆分配、无 GC 抖动，适合实时 DSP 与高频控制循环。
- **双命名规范并存**：同时提供兼容 npm `kalmanjs` 的 `KalmanFilter`（`R`/`Q`）与控制理论标准命名 `Kalman1D`（`q`/`r`）。
- **只读与可变方法严格分离**：查询方法不修改内部状态，更新方法明确标记可变接收者。
