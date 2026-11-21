# `cap:isotonic` — 零堆分配保序回归（PAVA）与概率校准库

`cap:isotonic` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的保序回归与概率校准工具链，包含 $O(n)$ 加权相邻违序合并算法（PAVA，支持非降/非增单调约束）、支持乱序与重复自变量合并的分段线性保序回归拟合器（`fitIsotonic` / `isotonicPredict`）以及二分类概率校准指标（`brierScore` 与等宽分箱 `expectedCalibrationError`）。

## 核心特性

- **100% 零堆分配**：PAVA 合并栈、原地堆排序索引缓冲、唯一节点输出与 ECE 分箱缓冲全部由调用方以切片传入。
- **$O(n)$ 加权 PAVA（`pavaIncreasing` / `pavaDecreasing`）**：精确求解加权最小二乘 $\min \sum w_i (f_i - y_i)^2$（满足单调约束与完整 KKT 最优性），支持空权重切片默认等权 `1.0`。
- **乱序 + 重复 `x` 保序拟合与预测**：`fitIsotonic` 内部采用 $O(n \log n)$ 原地稳定堆排序并按加权均值合并重复 `x` 节点；`isotonicPredict` 执行 $O(\log m)$ 二分查找分段线性插值并在区间外自动钳位。
- **概率校准评估**：内置 `brierScore` 与标准等宽分箱 `expectedCalibrationError`（ECE）。
- **完整防御校验**：自动拦截空数组、负/零/`NaN` 权重、非有限坐标、维度不匹配及缓冲不足。

---

## 导出清单

### 结构体

- `PavaWorkspace`：`{ block_sum_wy, block_sum_w, block_count, block_end }`（全部长度 `>= n`）
- `IsotonicModel`：`{ x_knots, y_knots, knot_count, increasing, y_min, y_max }`

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `pavaIncreasing(y, weights, y_out, ws): bool` | $O(n)$ 加权非降 PAVA（`weights` 为空默认等权） |
| `pavaDecreasing(y, weights, y_out, ws): bool` | $O(n)$ 加权非增 PAVA（`weights` 为空默认等权） |
| `fitIsotonic(x, y, weights, increasing, x_knots_out, y_knots_out, order_s, y_sorted_s, w_sorted_s, pava_ws): IsotonicModel?` | 乱序/含重复 `x` 的保序回归拟合 |
| `isotonicPredict(model, x_query): f64` | 分段线性插值预测（区间外自动钳位） |
| `brierScore(probs, y_binary): f64?` | Brier 评分 $\frac{1}{n}\sum (p_i - y_i)^2$ |
| `expectedCalibrationError(probs, y_binary, num_bins, bin_cnt_s, bin_prob_s, bin_acc_s): f64?` | 标准等宽分箱期望校准误差（ECE） |

---

## 使用示例

```typescript
import std from "std";
import {
    PavaWorkspace,
    pavaIncreasing,
    fitIsotonic,
    isotonicPredict,
    brierScore,
} from "cap:isotonic";

fn isotonicDemo() {
    const y = [1.0, 4.0, 2.0, 3.0, 5.0];
    const empty_w: [0]f64 = [];
    let y_out_arr: [5]f64 = std.mem.zeroes([5]f64);
    let swy: [5]f64 = std.mem.zeroes([5]f64);
    let sw: [5]f64 = std.mem.zeroes([5]f64);
    let sc: [5]usize = std.mem.zeroes([5]usize);
    let se: [5]usize = std.mem.zeroes([5]usize);
    let ws = PavaWorkspace {
        block_sum_wy: swy[0..],
        block_sum_w: sw[0..],
        block_count: sc[0..],
        block_end: se[0..],
    };
    let y_out: []f64 = y_out_arr[0..];
    // Produces [1.0, 3.0, 3.0, 3.0, 5.0]
    * = pavaIncreasing(y[0..], empty_w[0..], &y_out, &ws);

    const x = [3.0, 1.0, 2.0, 2.0, 5.0];
    let xk: [5]f64 = std.mem.zeroes([5]f64);
    let yk: [5]f64 = std.mem.zeroes([5]f64);
    let ord: [5]usize = std.mem.zeroes([5]usize);
    let ys: [5]f64 = std.mem.zeroes([5]f64);
    let ws_s: [5]f64 = std.mem.zeroes([5]f64);
    let xk_s: []f64 = xk[0..];
    let yk_s: []f64 = yk[0..];
    let ord_s: []usize = ord[0..];
    let ys_s: []f64 = ys[0..];
    let ws_sl: []f64 = ws_s[0..];
    if (model := fitIsotonic(x[0..], y[0..], empty_w[0..], true, &xk_s, &yk_s, &ord_s, &ys_s, &ws_sl, &ws)) {
        const pred = isotonicPredict(model, 2.5);
        * = pred;
    }
    const bs = brierScore([0.2, 0.8][0..], [0.0, 1.0][0..]);
    * = bs;
}
```
