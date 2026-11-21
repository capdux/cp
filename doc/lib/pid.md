# `cap:pid` — 工业离散 PID 控制器零分配库

零堆分配（Zero-Alloc）的纯 CP 工业控制与机器人离散 PID 控制器库，集成微分先行（Derivative-on-Measurement）、条件积分抗饱和（Clamping Anti-Windup）、一阶低通微分滤波、误差死区、输出限幅、无扰切换（Bumpless Transfer）、Ziegler-Nichols 整定与斜率限幅。

## 核心特性

- **零堆分配**：控制器结构体纯栈值布局，适用于硬实时嵌入式与高频控制回路。
- **条件积分抗饱和（Anti-Windup）**：当控制输出已达 `out_min`/`out_max` 限幅且误差继续推向饱和方向时冻结积分项，彻底消除积分饱和导致的响应滞后与过冲。
- **微分先行（`derivative_on_measurement`）**：对被控量 `-dy/dt` 而非误差 `de/dt` 求微，消除设定值阶跃变化引起的执行器微分冲击（Derivative Kick）。
- **一阶低通微分滤波（`tau_d`）**：采用离散指数平滑 $\alpha = \frac{dt}{\tau_d + dt}$ 抑制传感器高频噪声。
- **无扰切换（`setManualOutput`）**：从手动模式切回自动模式时预置积分器并同步当前测量值，输出平滑无跳变。

## 导出 API

### 数据模型

- `PidConfig { kp, ki, kd, out_min, out_max, tau_d, derivative_on_measurement, deadband }`
- `PidTerms { p, i, d, output, saturated }`
- `PidController`：
  - `PidController.init(cfg: PidConfig): PidController?`
  - `update(*_, setpoint: f64, measurement: f64, dt: f64): f64?`
  - `updateDetailed(*_, setpoint: f64, measurement: f64, dt: f64): PidTerms?`
  - `reset(*_)`
  - `setManualOutput(*_, output: f64, measurement: f64)`

### 辅助函数

- `zieglerNicholsPid(ku: f64, tu: f64): PidConfig?`：按临界比例度法计算 `kp = 0.6*ku`, `ki = 1.2*ku/tu`, `kd = 0.075*ku*tu`。
- `rateLimit(current: f64, target: f64, max_rate: f64, dt: f64): f64`：按最大变化率 `max_rate` 限制单步输出变化量。

## 使用示例

```cp
import { PidConfig, PidController, zieglerNicholsPid, rateLimit } from "cap:pid";

fn example() {
    const cfg = PidConfig {
        kp: 2.0,
        ki: 1.5,
        kd: 0.1,
        out_min: 0.0,
        out_max: 200.0,
        tau_d: 0.02,
        derivative_on_measurement: true,
        deadband: 0.0,
    };

    if (let ctrl := PidController.init(cfg)) {
        if (u := ctrl.update(100.0, 20.0, 0.01)) {
            * = rateLimit(0.0, u, 50.0, 0.01);
        }
    }
    * = zieglerNicholsPid(10.0, 2.0);
}
```
