# CP 标准库：数值计算、线性代数、优化、滤波与 AI 向量检索库手册 (`cap:math` / `cap.math`)

## 一、 架构总览与设计准则

`cap:math` 是 CP 语言官方内置的纯原生高性能数值计算与 AI 向量检索算子库，覆盖：
1. **向量与 4x4 矩阵 / 描述统计 / AI Embedding SIMD 检索与量化**（`vec.cp`, `stats.cp`, `embedding.cp`）
2. **稠密矩阵分解与特征值降维**（`lu.cp`, `qr.cp`, `svd.cp`, `pca.cp`）
3. **特殊数学函数与自适应数值积分**（`bessel.cp`, `gamma.cp`, `quadrature.cp`）
4. **线性规划、非线性优化、求根与全局启发式搜索**（`simplex.cp`, `bfgs.cp`, `nelder.cp`, `newton.cp`, `anneal.cp`, `pso.cp`, `ga.cp`）
5. **状态估计与传感器融合滤波**（`kalman.cp`, `ekf.cp`）

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，所有数值算法均采用**零堆分配（Zero-Alloc Workspace）或确定性显式分配器**设计，支持 `import ... from "cap:math"` 与 `import cap from "cap"; cap.math.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 核心职责 |
| :--- | :--- |
| `lib/math/vec.cp` | `Vec2`/`Vec3`/`Vec4` 值类型向量与列主序 `Mat4` 变换矩阵 |
| `lib/math/stats.cp` | Welford 描述统计 `StatsSummary`、`pearsonCorrelation`、`linearRegression` |
| `lib/math/embedding.cp` | 8-lane SIMD 点积/范数/余弦/欧氏/曼哈顿距离、Softmax、Top-K 与对称 Int8 量化 |
| `lib/math/lu.cp` | Doolittle 部分主元 LU 分解、线性求解、行列式、逆矩阵与 `O(n)` Thomas 三对角求解 |
| `lib/math/qr.cp` | Householder QR 分解、正交基构造与线性最小二乘求解 |
| `lib/math/svd.cp` | 单边 Jacobi 奇异值分解（SVD）、截断最小二乘求解与 Moore-Penrose 伪逆 |
| `lib/math/pca.cp` | 对称矩阵经典 Jacobi 特征值求解与主成分分析（PCA）拟合/变换/重构 |
| `lib/math/bessel.cp` | 第一/二类贝塞尔函数 `J0/J1/Jn/Y0/Y1/Yn` 与修正贝塞尔 `I0/I1/I0e/I1e/K0/K1/K0e/K1e` |
| `lib/math/gamma.cp` | `lgamma`/`tgamma`/`digamma`、正则化不完全 Gamma/Beta、`erf`/`erfc`、对数阶乘与组合数 |
| `lib/math/quadrature.cp` | 5/10 点 Gauss-Legendre、复合 Simpson、自适应 G7-K15 与 Romberg 外推积分 |
| `lib/math/simplex.cp` | 两阶段单纯形法线性规划（含 Bland 防循环规则与对偶影子价格） |
| `lib/math/bfgs.cp` | 拟牛顿法完整 BFGS、有限内存 L-BFGS、强 Wolfe 线搜索与中心差分梯度 |
| `lib/math/nelder.cp` | 无导数 Nelder-Mead 单纯形优化、1D 黄金分割极小化与 Brent 1D 求根 |
| `lib/math/newton.cp` | 阻尼多维 Newton-Raphson、Levenberg-Marquardt 非线性最小二乘与 1D Newton/割线求根 |
| `lib/math/anneal.cp` | 连续盒约束与排列 2-opt/Swap 模拟退火（指数/线性/对数冷却调度） |
| `lib/math/pso.cp` | 粒子群优化（PSO）与差分进化（DE/rand/1/bin）全局启发式优化 |
| `lib/math/ga.cp` | 实数编码遗传算法与顺序交叉（OX）排列遗传算法 |
| `lib/math/kalman.cp` | Joseph 形式 1D/2D 卡尔曼滤波器与 Alpha-Beta 传感器融合滤波 |
| `lib/math/ekf.cp` | 数值 Jacobian、Joseph 形式扩展卡尔曼滤波（EKF）与 Merwe-Scaled 无迹卡尔曼滤波（UKF） |
| `lib/math/index.cp` | `cap:math` 与 `cap.math` 统一零冲突公开导出入口（共 163 符号） |

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
  - `dotF32(a, b): f32`、`l2NormF32(v): f32`、`normalizeL2(v: []f32)`
  - `cosineSimilarityF32(a, b): f32`、`euclideanDistanceF32(a, b): f32`、`manhattanDistanceF32(a, b): f32`
- **注意力与检索排序**：
  - `softmaxInPlace(logits: []f32)`、`argMaxF32(v: []const f32): usize?`、`topKIndices(alloc, scores: []const f32, k: usize): ([]usize)!`
- **对称 Int8 量化**：
  - `QuantizedVecI8`、`quantizeSymmetricI8`、`dequantizeSymmetricI8`、`dotQuantizedI8`

### 4. 稠密矩阵分解与特征值降维（`lu.cp` / `qr.cp` / `svd.cp` / `pca.cp`）

- **LU 分解（`lu.cp`）**：`LuInfo`、`luFactorize`、`luSolve`、`luDeterminant`、`luInverse`、`matVecResidualInf`、`solveTridiagonal`
- **QR 分解（`qr.cp`）**：`QR_DEFAULT_TOL`、`qrFactorize`、`qrApplyQt`、`qrFormQ`、`qrExtractR`、`qrSolveLeastSquares`、`qrOrthogonalityError`、`qrReconstructionError`
- **SVD 奇异值分解（`svd.cp`）**：`SvdResult`、`svdJacobi`、`svdSolve`、`svdPseudoinverse`、`svdReconstructionError`
- **对称特征值与 PCA（`pca.cp`）**：`jacobiEigenSymmetric`、`PcaFitResult`、`pcaFit`、`pcaTransform`、`pcaInverseTransform`、`pcaReconstructionMse`

### 5. 特殊数学函数与数值积分（`bessel.cp` / `gamma.cp` / `quadrature.cp`）

- **贝塞尔函数（`bessel.cp`）**：`j0`, `j1`, `jn`, `y0`, `y1`, `yn`, `besselI0`, `besselI1`, `i0e`, `i1e`, `k0`, `k1`, `k0e`, `k1e`
- **Gamma / Beta / Erf（`gamma.cp`）**：`lgamma`, `tgamma`, `digamma`, `gammaP`, `gammaQ`, `lbeta`, `beta`, `betainc`, `erf`, `erfc`, `logFactorial`, `logBinomial`
- **数值积分（`quadrature.cp`）**：`IntegrandFn`, `Gk15Estimate`, `QuadResult`, `gaussLegendre5`, `gaussLegendre10`, `compositeSimpson`, `gaussKronrod15`, `adaptiveGk15`, `romberg`

### 6. 数学优化与非线性求解（`simplex.cp` / `bfgs.cp` / `nelder.cp` / `newton.cp` / `anneal.cp` / `pso.cp` / `ga.cp`）

- **线性规划（`simplex.cp`）**：`ConstraintKind`, `SimplexStatus`, `SimplexResult`, `SimplexWorkspace`, `simplexRequiredTableauLen`, `solveGeneral`, `solveMaxLe`, `solveMinLe`
- **拟牛顿优化（`bfgs.cp`）**：`ValueGradFn`, `ValueOnlyFn`, `BfgsConfig`, `BfgsResult`, `BfgsWorkspace`, `LbfgsWorkspace`, `bfgsMinimize`, `lbfgsMinimize`, `finiteDiffGradient`
- **无导数优化与 1D 求根（`nelder.cp`）**：`ObjectiveFn`, `ScalarFn`, `NelderConfig`, `NelderWorkspace`, `NelderResult`, `Minimize1DResult`, `Root1DResult`, `nelderMead`, `goldenSectionMin`, `brentRoot`
- **Newton 与 Levenberg-Marquardt（`newton.cp`）**：`ResidualFn`, `NewtonConfig`, `NewtonResult`, `NewtonWorkspace`, `LmConfig`, `LmResult`, `LmWorkspace`, `RootResult`, `newtonSystem`, `levenbergMarquardt`, `newton1D`, `secant1D`
- **模拟退火（`anneal.cp`）**：`CoolingSchedule`, `RealEnergyFn`, `PermEnergyFn`, `AnnealConfig`, `AnnealResult`, `temperatureAt`, `metropolisAccept`, `annealContinuous`, `annealPermutation`
- **粒子群与差分进化（`pso.cp`）**：`GlobalOptResult`, `PsoConfig`, `PsoWorkspace`, `DeConfig`, `DeWorkspace`, `psoMinimize`, `deMinimize`
- **遗传算法（`ga.cp`）**：`RealObjectiveFn`, `PermObjectiveFn`, `RealGaConfig`, `PermGaConfig`, `GaResult`, `RealGaWorkspace`, `PermGaWorkspace`, `isValidPermutation`, `orderCrossover`, `realGaMinimize`, `permGaMinimize`

### 7. 状态估计与滤波（`kalman.cp` / `ekf.cp`）

- **线性卡尔曼与 Alpha-Beta（`kalman.cp`）**：`Kalman1D`, `Kalman2D`, `AlphaBetaFilter`
- **非线性 EKF / UKF（`ekf.cp`）**：`TransitionFn`, `MeasurementFn`, `EkfWorkspace`, `UkfParams`, `numericalJacobianF`, `numericalJacobianH`, `ekfPredict`, `ekfUpdate`, `generateSigmaPoints`, `ukfPredict`

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
    luFactorize,
    luSolve,
    erf,
    Kalman1D,
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

    // 特殊函数与滤波器
    const e1 = erf(1.0);
    let kf = Kalman1D.init(0.0, 1.0, 0.01, 0.1);
    * = kf.update(1.2);
    * = e1;

    // 矩阵 LU 求解
    let lu_a: [4]f64 = [2.0, 1.0, 5.0, 7.0];
    let piv: [2]usize = [0, 0];
    let rhs: [2]f64 = [11.0, 13.0];
    let sol: [2]f64 = [0.0, 0.0];
    if (luFactorize(2, &lu_a, &piv)) |_| {
        * = luSolve(2, &lu_a, &piv, &rhs, &sol);
    }

    // 动态懒加载挂载等价用法
    const sim = cap.math.cosineSimilarityF32(&vec, rec);
    * = sim;
}
```
