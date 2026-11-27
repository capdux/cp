[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# kalman-filters

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:kalman-filters` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `kalman-filters` → CP 包名 `kalman-filters`）

Zero-allocation 1D scalar (`Kalman1D`) and 2D constant-velocity (`Kalman2D`) Kalman filters with Joseph-form covariance update for CP — clean-room MIT parity with Rust `kalman-filters` and `kfilter`.

纯 CP 零堆分配卡尔曼滤波器引擎，提供一维标量平滑与二维恒速位置-速度追踪（采用 Joseph 形式数值稳定协方差更新），适用于 GPS/IMU 传感器去噪、物联网时序平滑与实时目标追踪。

## Install / 安装

```bash
cap add kalman-filters
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `Kalman1D` | `init(x, p, q, r)`, `predict(*_)`, `update(*_, z) -> f64`, `step(*_, z) -> f64`, `estimate()`, `variance()`, `gain()`, `reset(*_, x, p)` | 1D scalar Kalman filter |
| `Kalman2D` | `init(pos, vel, q_pos, q_vel, r_pos)`, `predict(*_, dt)`, `updatePosition(*_, z) -> f64`, `position()`, `velocity()` | 2D constant-velocity `[pos, vel]` filter with Joseph covariance update |

## Usage / 使用示例

```cp
import { Kalman1D, Kalman2D } from "kalman-filters";

let kf = Kalman1D.init(0.0, 1.0, 0.01, 0.25);
const est = kf.step(10.2);

let tracker = Kalman2D.init(0.0, 0.0, 0.01, 0.01, 0.1);
tracker.predict(1.0);
const pos = tracker.updatePosition(5.1);
```
