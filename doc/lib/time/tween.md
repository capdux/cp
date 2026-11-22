# CP 标准库：缓动曲线、关键帧轨道与弹簧阻尼物理动画引擎 (`cap:tween` / `cap.tween`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:time`，导入请使用 `import { ... } from "cap:time"`。

## 一、 架构总览与设计准则

`cap:tween` 是 CP 语言官方内置的纯原生、零 C 依赖的 **运动学缓动插值、关键帧轨道与弹簧阻尼物理动画引擎**，面向 UI 动效（配合 `cap:tui` / `cap:svg`）、游戏实体运动（配合 `cap:ecs`）与连续物理动画场景设计。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:time"` 与 `import cap from "cap"; cap.tween.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/tween/tween.cp` | `// Source: cp/lib/tween/tween.cp (...)` | `TweenError`、`EaseKind`、`Easing`、`Tween`、`Keyframe`、`KeyframeTrack` 与 `SpringDamper` 核心实现 |
| `lib/tween/index.cp` | `// Source: cp/lib/tween/index.cp (...)` | `cap:tween` 与 `cap.tween` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与曲线枚举 (`TweenError` & `EaseKind`)

```typescript
pub err TweenError {
    InvalidKeyframeOrder,
    EmptyTrack,
    InvalidSpringParams,
    InvalidDuration,
    OutOfMemory,
}
```

`EaseKind` 提供完整 31 种经典 Robert Penner 缓动曲线：
- 线性：`Linear`
- 二次/三次/四次/五次幂：`InQuad` / `OutQuad` / `InOutQuad`、`InCubic` / `OutCubic` / `InOutCubic`、`InQuart` / `OutQuart` / `InOutQuart`、`InQuint` / `OutQuint` / `InOutQuint`
- 正弦与指数：`InSine` / `OutSine` / `InOutSine`、`InExpo` / `OutExpo` / `InOutExpo`
- 圆弧：`InCirc` / `OutCirc` / `InOutCirc`
- 回拉过冲：`InBack` / `OutBack` / `InOutBack`
- 弹性振荡：`InElastic` / `OutElastic` / `InOutElastic`
- 弹跳：`InBounce` / `OutBounce` / `InOutBounce`

### 2. 静态缓动与插值工具 (`Easing`)

- `Easing.sample(kind: EaseKind, t: f64): f64` — 对 `t` 采样缓动曲线；严格保证 `t <= 0.0` 返回 `0.0`、`t >= 1.0` 返回 `1.0`（`NaN` 返回 `0.0`）。
- `Easing.clamp01(t: f64): f64` — 将标量钳位到 `[0.0, 1.0]`。
- `Easing.lerp(a: f64, b: f64, t: f64): f64` — 线性插值 `a + (b - a) * t`。
- `Easing.lerpEased(a: f64, b: f64, t: f64, kind: EaseKind): f64` — 先按 `kind` 采样再线性插值。
- `Easing.smoothstep(edge0: f64, edge1: f64, x: f64): f64` — 三次 Hermite 平滑阶跃函数。

### 3. 单段补间控制器 (`Tween`)

- `Tween.init(duration_ms: f64, kind: EaseKind, yoyo: bool, repeat_count: u32): Tween ! TweenError` — 创建补间控制器；`duration_ms <= 0.0` 或非有限值返回 `InvalidDuration`。`repeat_count == 0` 表示单次播放，`N` 表示额外重复 `N` 次（共 `N + 1` 趟）。
- `update(*_, dt_ms: f64): f64` — 推进 `dt_ms` 毫秒并返回当前缓动进度。
- `progress(): f64` — 读取当前缓动进度（不推进时间）。
- `isFinished(): bool` — 查询全部趟次是否已结束。
- `reset(*_)` — 重置已用时间、已完成趟次与结束状态。

### 4. 多段关键帧轨道 (`Keyframe` & `KeyframeTrack`)

- `Keyframe`：`{ time: f64, value: f64, ease: EaseKind }`，其中 `ease` 控制该关键帧到下一关键帧的出射段插值曲线。
- `KeyframeTrack.init(alloc: std.mem.Allocator): KeyframeTrack` / `deinit(*_)` — 初始化与释放轨道内存。
- `addKeyframe(*_, time: f64, value: f64, ease: EaseKind): ! TweenError` — 追加关键帧，强制 `time` 严格单调递增，否则返回 `InvalidKeyframeOrder`。
- `len(): usize` — 返回关键帧数量。
- `evaluate(time: f64): f64 ! TweenError` — 空轨道返回 `EmptyTrack`；首尾之外自动钳位到首/尾关键帧值；区间内通过二分查找定位分段并执行分段缓动插值。

### 5. 连续时间弹簧阻尼物理系统 (`SpringDamper`)

基于单位质量二阶动力学方程 $a = -k(x - x_{\text{target}}) - c v$，内部采用**稳定性受控自适应子步长（$\le \min(1/240, 0.25/\sqrt{k})$）半隐式欧拉积分**，彻底防止大 `dt` 或高刚度下的数值发散。

- `SpringDamper.init(stiffness: f64, damping: f64, initial_val: f64): SpringDamper ! TweenError` — 校验 `stiffness > 0.0` 且 `damping >= 0.0`，否则返回 `InvalidSpringParams`。
- `SpringDamper.initWithRatio(omega: f64, damping_ratio: f64, initial_val: f64): SpringDamper ! TweenError` — 按自然角频率 `omega`（$\text{rad/s}$）与阻尼比 `damping_ratio`（$\zeta$）构造。
- `setTarget(*_, target: f64)` — 设置平衡目标位置。
- `step(*_, dt_sec: f64)` — 推进物理仿真 `dt_sec` 秒。
- `value(): f64` / `velocity(): f64` — 读取当前位置与速度。
- `naturalFrequency(): f64` / `dampingRatio(): f64` — 读取自然角频率与无量纲阻尼比。
- `isSettled(pos_eps: f64, vel_eps: f64): bool` — 判断位置误差与速度是否均已收敛到阈值内。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { EaseKind, Easing, Tween, KeyframeTrack, SpringDamper } from "cap:time";

pub fn demoTween(): ! {
    const alloc = std.heap.page_allocator;

    // 1. Easing & Tween
    let tw = Tween.init(200.0, EaseKind.OutCubic, true, 1)?;
    const p1 = tw.update(100.0);
    * = p1;

    // 2. KeyframeTrack
    let track = KeyframeTrack.init(alloc);
    defer track.deinit();
    track.addKeyframe(0.0, 0.0, EaseKind.InQuad)?;
    track.addKeyframe(1.0, 100.0, EaseKind.Linear)?;
    const v = track.evaluate(0.5)?;
    * = v;

    // 3. SpringDamper & cap.tween 动态挂载
    let spring = cap.tween.SpringDamper.initWithRatio(12.0, 1.0, 0.0)?;
    spring.setTarget(100.0);
    spring.step(0.016);
}
```
