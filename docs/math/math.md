# CP 标准库：数值计算与 AI 向量检索算子库手册 (`cap:math` / `cap.math`)

## 一、 架构总览与设计准则

`cap:math` 是 CP 语言官方内置的纯原生高性能数值计算与 AI 向量检索算子库，覆盖 **2D/3D/4D 向量与列主序 4x4 矩阵线性代数**、**Welford 数值稳定描述统计/分位数/皮尔逊相关与最小二乘线性回归**，以及 **8-lane SIMD 加速 Embedding 相似度、数值稳定 Softmax、O(N log k) Top-K 检索与对称 Int8 量化算子**。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:math"` 与 `import cap from "cap"; cap.math.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/math/vec.cp` | `// Source: cp/lib/math/vec.cp (2D/3D/4D Vector & Mat4 Linear Algebra)` | `Vec2`/`Vec3`/`Vec4` 值类型向量与列主序 `Mat4` 变换矩阵 |
| `lib/math/stats.cp` | `// Source: cp/lib/math/stats.cp (Descriptive Statistics & Linear Regression)` | `StatsSummary`、`Stats.summarize`、`pearsonCorrelation`、`linearRegression` |
| `lib/math/embedding.cp` | `// Source: cp/lib/math/embedding.cp (AI Embedding Similarity, Softmax & Int8 Quantization)` | SIMD 点积/范数/余弦/欧氏/曼哈顿距离、Softmax、Top-K 与对称 Int8 量化 |
| `lib/math/index.cp` | `// Source: cp/lib/math/index.cp (Unified Exports for cap:math)` | `cap:math` 与 `cap.math` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 向量与 4x4 矩阵线性代数（`vec.cp`）

- **`Vec2` / `Vec3` / `Vec4`**（`f32` 值类型）：
  - 构造：`init(...)`、`zero()`、`splat(v)`
  - 算术与几何：`add(other)`、`sub(other)`、`scale(s)`、`dot(other)`、`lengthSq()`、`length()`、`normalize()`、`lerp(other, t)`、`distance(other)`、`approxEql(other, eps)`
  - `Vec3` 专属：`cross(other)`（右手系三维叉积）
- **`Mat4`**（列主序 `[16]f32`，`data[col * 4 + row]`）：
  - 构造：`identity()`、`fromCols(c0, c1, c2, c3)`、`translation(tx, ty, tz)`、`scaling(sx, sy, sz)`、`rotationZ(radians)`
  - 运算：`get(row, col)`、`mul(other)`、`mulVec4(v)`、`transpose()`、`approxEql(other, eps)`

### 2. 描述统计与线性回归（`stats.cp`）

- **`StatsErr`**：`EmptyInput`、`LengthMismatch`、`ZeroVariance`
- **`StatsSummary`**：`count`、`min`、`max`、`sum`、`mean`、`variance`（总体方差）、`stddev`（总体标准差）、`median`、`p95`、`p99`
- **`Stats.summarize(alloc, data: []const f64): StatsSummary!`**
  - 使用 Welford 单遍在线算法计算均值与 `M2`，并在临时副本上排序后以线性插值法（`R-7`）计算 `median` / `p95` / `p99`。
- **`pearsonCorrelation(x, y): f64!`**（亦提供 `Stats.pearsonCorrelation`）：计算皮尔逊相关系数 `r ∈ [-1, 1]`。
- **`linearRegression(x, y): LinearRegressionResult!`**（亦提供 `Stats.linearRegression`）：普通最小二乘回归，返回 `{ slope, intercept, r_squared }` 及 `predict(x)` 方法。

### 3. AI Embedding 检索与量化算子（`embedding.cp`）

- **SIMD 向量算子**（基于 `@Vector(8, f32)` 向量化 + 标量尾部收尾）：
  - `dotF32(a, b): f32`
  - `l2NormF32(v): f32`
  - `normalizeL2(v: []f32)`（原地 L2 归一化）
  - `cosineSimilarityF32(a, b): f32`（单遍同步归约点积与双范数）
  - `euclideanDistanceF32(a, b): f32`
  - `manhattanDistanceF32(a, b): f32`
- **注意力与检索排序**：
  - `softmaxInPlace(logits: []f32)`：先减去全局最大值 `max(logits)` 防止 `exp` 上溢，再归一化使概率和为 1.0。
  - `argMaxF32(v: []const f32): usize?`
  - `topKIndices(alloc, scores: []const f32, k: usize): ([]usize)!`：基于原地最小堆的 `O(N log k)` Top-K 检索，按得分降序返回下标切片（并列得分优先保留较小下标）。
- **对称 Int8 量化**：
  - `QuantizedVecI8`：`{ scale: f32, data: []i8 }`，提供 `deinit(*_, alloc)`。
  - `quantizeSymmetricI8(alloc, v: []const f32): QuantizedVecI8!`：`scale = max(|v|) / 127.0`，将权重/Embedding 线性映射到 `[-127, 127]`。
  - `dequantizeSymmetricI8(alloc, q: QuantizedVecI8): ([]f32)!`：按 `q.data[i] * q.scale` 反量化回 `f32`。
  - `dotQuantizedI8(a, b): f32`：整型 `i64` 累加后乘以 `a.scale * b.scale` 计算近似点积。

---

## 三、 使用示例

```typescript
import cap from "cap";
import {
    Vec3,
    Mat4,
    Stats,
    cosineSimilarityF32,
    softmaxInPlace,
    topKIndices,
    quantizeSymmetricI8,
    dequantizeSymmetricI8,
} from "cap:math";

pub fn demoMath(): !void {
    const a = Vec3.init(1.0, 0.0, 0.0);
    const b = Vec3.init(0.0, 1.0, 0.0);
    const n = a.cross(b);

    const m = Mat4.translation(10.0, 20.0, 0.0).mul(Mat4.scaling(2.0, 3.0, 1.0));
    * = m;
    * = n;

    const samples = [10.0, 20.0, 30.0, 40.0, 50.0];
    const summary = Stats.summarize(std.heap.page_allocator, &samples)?;
    * = summary;

    let logits = [1000.0, 1001.0, 1002.0];
    softmaxInPlace(&logits);

    const scores = [0.1, 0.9, 0.4, 0.8];
    const top2 = topKIndices(std.heap.page_allocator, &scores, 2)?;
    defer std.heap.page_allocator.free(top2);

    const vec = [0.25, -0.5, 1.0, -0.75];
    let q = quantizeSymmetricI8(std.heap.page_allocator, &vec)?;
    defer q.deinit(std.heap.page_allocator);
    const rec = dequantizeSymmetricI8(std.heap.page_allocator, q)?;
    defer std.heap.page_allocator.free(rec);

    // 动态懒加载挂载等价用法
    const sim = cap.math.cosineSimilarityF32(&vec, rec);
    * = sim;
}
```
