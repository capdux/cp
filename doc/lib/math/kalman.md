# `cap:kalman` — 1D/2D 卡尔曼滤波器与 Alpha-Beta 跟踪滤波器

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

纯 CP、零堆分配的传感器融合与状态估计标准库，内置数值稳定的 Joseph 形式协方差更新、2D 恒速 `[pos, vel]` 连续白噪声加速度模型、马氏距离野值门控以及轻量 Alpha-Beta 跟踪滤波器。

## 导入

```cp
import {
    Kalman1D,
    Kalman2D,
    AlphaBetaFilter,
} from "cap:math";
```

## 核心 API

- **`Kalman1D`**（1D 标量卡尔曼滤波器）：
  - `Kalman1D.init(x0, p0, q, r): Kalman1D?` — 校验 `p0 >= 0, q >= 0, r > 0` 且有限。
  - `predict(*_, control: f64, b: f64)`、`update(*_, z: f64): f64`（Joseph 形式方差更新）、`step(*_, z: f64): f64`。
- **`Kalman2D`**（2D 恒速 `[pos, vel]` 卡尔曼滤波器）：
  - `Kalman2D.init(pos0, vel0, p_pos0, p_vel0, accel_var, meas_var): Kalman2D?`
  - `predict(*_, dt: f64): bool` — 连续白噪声加速度过程噪声 `Q`。
  - `updatePosition(*_, z_pos: f64): f64` / `updateVelocity(*_, z_vel: f64, vel_var: f64): f64?` — 2×2 Joseph 协方差更新。
  - `mahalanobisSq(z_pos: f64): f64` — 新息马氏距离平方（离群观测门控）。
- **`AlphaBetaFilter`**：
  - `AlphaBetaFilter.init(pos0, vel0, alpha, beta): AlphaBetaFilter?` — 校验 `0 < alpha < 1 && 0 < beta <= 2`。
  - `update(*_, z_pos: f64, dt: f64): f64?`。

## 示例

```cp
import { Kalman2D } from "cap:math";

fn trackStep(z_pos: f64, dt: f64): f64? {
    let kf = Kalman2D.init(0.0, 0.0, 10.0, 10.0, 0.1, 1.0) ?? return null;
    if (!kf.predict(dt)) {
        return null;
    }
    return kf.updatePosition(z_pos);
}
```
