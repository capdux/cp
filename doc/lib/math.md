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
| `lib/math/biquad.cp` | RBJ Audio EQ Biquad IIR 滤波器族、SOS 级联与 Goertzel 单频音调检测 |
| `lib/math/iir.cp` | Butterworth/Chebyshev-I 双线性 SOS 级联 IIR 设计与零相位 `sosFiltFilt` |
| `lib/math/fir.cp` | Hann/Hamming/Blackman 窗、Windowed-Sinc 低通 FIR 与轻量 `Biquad` |
| `lib/math/fft.cp` | Cooley-Tukey Radix-2 FFT/IFFT、窗函数、功率谱密度（PSD）与卷积 |
| `lib/math/wavelet.cp` | Haar、Daubechies-4（db2）与 JPEG 2000 无损 CDF 5/3 DWT/IDWT 及阈值去噪 |
| `lib/math/savitzky.cp` | Savitzky-Golay 多项式平滑/求导与 Cleveland LOESS 局部加权回归 |
| `lib/math/holt.cp` | 指数平滑预测（SES、Holt 线性/阻尼、Holt-Winters 季节性）与误差指标 |
| `lib/math/cusum.cp` | 在线变点检测（CUSUM、Page-Hinkley、EWMA 控制图、流式 Z-Score） |
| `lib/math/dtw.cp` | 1D/多维动态时间规整（DTW）、回溯对齐、Keogh 包络与 LB_Keogh 下界 |
| `lib/math/hmm.cp` | 离散隐马尔可夫模型（对数域 Viterbi、Rabiner 缩放前向-后向、Baum-Welch EM） |
| `lib/math/markov.cp` | 离散马尔可夫链稳态分布、吸收链基本矩阵、轨迹采样与熵率 |
| `lib/math/particle.cp` | 序列蒙特卡洛粒子滤波（系统/分层/多项重采样、有效样本量 ESS） |
| `lib/math/hilbert.cp` | 2D Hilbert 空间填充曲线与 2D/3D Morton Z-order 编解码 |
| `lib/math/spline.cp` | 自然/钳位三次样条、单调保形 PCHIP 样条与 Catmull-Rom 插值 |
| `lib/math/ode.cp` | RK2 中点、经典 RK4、自适应 Dormand-Prince RK45 与 Velocity Verlet 积分器 |
| `lib/math/cholesky.cp` | SPD 矩阵 Cholesky `LL^T` 与无开方 `LDL^T` 分解、求解、对数行列式与求逆 |
| `lib/math/simdStats.cp` | 原生 8-lane SIMD（`@dot`/`@l2sq`/`@fma`/`slice.split`）零堆描述统计、相关/余弦/欧氏距离、Z-Score、Softmax、GEMV 与 AXPY |
| `lib/math/index.cp` | `cap:math` 与 `cap.math` 统一零冲突公开导出入口 |

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

### 8. 数字信号处理与频谱分析（`biquad.cp` / `iir.cp` / `fir.cp` / `fft.cp` / `wavelet.cp`）

- **Biquad 与 Goertzel（`biquad.cp`）**：`FilterType`, `BiquadCoeffs`, `BiquadFilter`, `FreqResponse`, `GoertzelResult`, `designBiquad`, `processSosBlock`, `frequencyResponse`, `goertzelDetect`
- **IIR 级联与零相位滤波（`iir.cp`）**：`SosSection`, `SosState`, `designButterworthLowpass`, `designButterworthHighpass`, `designChebyshev1Lowpass`, `sosFilter`, `sosFiltFilt`, `sosMagnitudeDb`
- **FIR 窗函数与滤波器（`fir.cp`）**：`windowHann`, `windowHamming`, `windowBlackman`, `designLowpassFir`, `FirFilter`, `Biquad`
- **快速傅里叶变换（`fft.cp`）**：`FftError`, `Complex`, `WindowKind`, `FrequencyPeak`, `Fft`
- **离散小波变换（`wavelet.cp`）**：`haarForward1d`, `haarInverse1d`, `haarForwardMulti1d`, `haarInverseMulti1d`, `db4Forward1d`, `db4Inverse1d`, `cdf53ForwardI32`, `cdf53InverseI32`, `softThreshold`, `hardThreshold`, `signalEnergy`

### 9. 时间序列平滑、预测、变点检测与对齐（`savitzky.cp` / `holt.cp` / `cusum.cp` / `dtw.cp`）

- **Savitzky-Golay 与 LOESS（`savitzky.cp`）**：`BoundaryMode`, `SavgolWorkspace`, `LoessConfig`, `savgolCoeffs`, `savgolFilter`, `loessSmooth1D`
- **指数平滑预测（`holt.cp`）**：`SeasonalityKind`, `SesResult`, `HoltConfig`, `HoltResult`, `HoltWintersConfig`, `HoltWintersResult`, `ForecastMetrics`, `sesForecast`, `holtForecast`, `holtWintersForecast`, `evaluateForecast`
- **在线变点检测（`cusum.cp`）**：`ShiftDirection`, `ChangePoint`, `CusumDetector`, `PageHinkleyDetector`, `EwmaControlChart`, `StreamingZScore`, `scanCusum`
- **动态时间规整（`dtw.cp`）**：`DTW_INF`, `WarpPair`, `DtwPathResult`, `dtwDistance1D`, `dtwAlign1D`, `dtwDistanceMulti`, `computeKeoghEnvelope`, `lbKeogh`, `zNormalize`

### 10. 随机过程、粒子滤波与空间填充曲线（`hmm.cp` / `markov.cp` / `particle.cp` / `hilbert.cp`）

- **离散 HMM（`hmm.cp`）**：`HmmModel`, `ViterbiResult`, `viterbiDecode`, `forwardScaled`, `backwardScaled`, `posteriorStates`, `baumWelchStep`
- **离散马尔可夫链（`markov.cp`）**：`isStochasticMatrix`, `stepDistribution`, `transitionPower`, `StationaryResult`, `stationaryPowerIter`, `absorbingFundamental`, `expectedAbsorptionSteps`, `absorptionProbabilities`, `sampleTrajectory`, `entropyRate`
- **粒子滤波（`particle.cp`）**：`ResampleKind`, `PropagateFn`, `LogLikelihoodFn`, `normalizeLogWeights`, `effectiveSampleSize`, `resampleIndices`, `ParticleFilter`
- **Hilbert / Morton 曲线（`hilbert.cp`）**：`HILBERT_MIN_ORDER`, `HILBERT_MAX_ORDER`, `MORTON3D_MAX_COORD`, `Point2D`, `Point3D`, `hilbert2dMaxIndex`, `hilbert2dEncode`, `hilbert2dDecode`, `morton2dEncode`, `morton2dDecode`, `morton3dEncode`, `morton3dDecode`, `manhattanDist2d`

### 11. 样条插值、常微分方程与 SPD 分解（`spline.cp` / `ode.cp` / `cholesky.cp`）

- **三次样条与 PCHIP（`spline.cp`）**：`CubicSegment`, `CubicSpline`, `buildNaturalCubicSpline`, `buildClampedCubicSpline`, `buildPchipSpline`, `catmullRom1d`, `catmullRom1dDeriv`
- **ODE 数值积分（`ode.cp`）**：`OdeRhsFn`, `AccelFn`, `Rk45Result`, `rk2MidpointStep`, `rk4Step`, `rk4Integrate`, `rk45Step`, `rk45Integrate`, `verletStep`
- **Cholesky 与 LDL^T（`cholesky.cp`）**：`choleskyDecompose`, `forwardSubstitute`, `backwardSubstituteT`, `choleskySolve`, `choleskyLogDet`, `choleskyDet`, `choleskyInvert`, `ldltDecompose`, `ldltSolve`

### 12. 128 位定点高精度十进制与 ISO 4217 货币引擎（`decimal.cp` / `decimalMoney.cp`）

- **128 位定点十进制（`decimal.cp`）**：`MAX_SCALE`, `DecimalError`, `RoundingMode`（HalfUp/HalfEven/Floor/Ceil/Trunc）, `Decimal`（精确十进制加减乘除、科学计数法解析与归一化）。
- **ISO 4217 货币引擎（`decimalMoney.cp`）**：`MoneyError`, `Money`（币种隔离算术与分精度无损分摊）。
- **参考归档（不导出）**：`lib/math/decimal_fp_.cp`（原 `lib/decimal_fp`，与 `decimal.cp` 功能重叠，首行带 `// Reference Only:` 声明，保留供对比分析）。

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

### 原生 SIMD 向量化统计与线性代数极速算子示例 (`simdStats.cp`)

```cp
import {
    simdSummaryStatsF32,
    simdPearsonCorrF32,
    simdCosineSimF32,
    simdEuclideanDistF32,
    simdZScoreNormalizeF32,
    simdSoftmaxF32,
    simdMatVecF32,
    simdAxpyF32,
} from "cap:math";

pub fn demoSimdStats() {
    const x: [8]f32 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0];
    const y: [8]f32 = [2.0, 4.0, 6.0, 8.0, 10.0, 12.0, 14.0, 16.0];

    // 1. Welford 防抵消描述统计 + 8-lane SIMD @dot L2 范数
    if (st := simdSummaryStatsF32(x[0..])) {
        * = st.mean;
        * = st.l2_norm;
    }

    // 2. 皮尔逊相关 / SIMD 余弦相似度 / SIMD 欧氏距离
    * = simdPearsonCorrF32(x[0..], y[0..]);   // 1.0
    * = simdCosineSimF32(x[0..], y[0..]);     // 1.0
    * = simdEuclideanDistF32(x[0..], y[0..]);

    // 3. Z-Score 标准化 & 数值稳定 Softmax
    let z_out: [8]f32 = [0.0; 8];
    * = simdZScoreNormalizeF32(x[0..], z_out[0..]);
    let probs: [3]f32 = [0.0; 3];
    const logits: [3]f32 = [1000.0, 1001.0, 1002.0];
    * = simdSoftmaxF32(logits[0..], probs[0..]);

    // 4. 基于 slice.split + @dot 的行主序 GEMV 与硬件 @fma AXPY
    const mat: [6]f32 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0];
    const v: [3]f32 = [1.0, 0.0, -1.0];
    let mv_out: [2]f32 = [0.0; 2];
    * = simdMatVecF32(mat[0..], 2, 3, v[0..], mv_out[0..]); // [-2.0, -2.0]
    * = simdAxpyF32(0.5, v[0..], probs[0..]);
}
```

### 无上限外部切片概率分布、假设检验与 OLS/Ridge 回归统计套件 (`statsDist.cp`)

面向科学计算、A/B 实验统计推断与特征回归建模的 100% 纯 CP 零动态堆分配统计套件。全部样本统计、假设检验、相关性与回归算子直接作用于调用方传入的任意长度 `[]const f64` 外部切片，无缝承接数万级样本：

- **常量与错误集**：
  - `STATS_MAX_FEATURES = 16`
  - `StatsDistError`（`EmptySample | InsufficientSamples | InvalidParameter | LengthMismatch | ZeroVariance | SingularMatrix | BufferTooSmall`）
- **高精度特殊函数**：
  - `logGamma(x)`（9 项 Lanczos 近似，精度 `< 1e-13`）
  - `erfApprox(x)` / `erfcApprox(x)`（基于 `P(0.5, x^2)`/`Q(0.5, x^2)`，全定义域无尾部相消）
  - `normalInvCdf(p)`（Acklam 分段有理逼近 + Halley 精化，精度 `< 1e-13`）
  - `regularizedIncompleteGammaP(a, x)` / `regularizedIncompleteGammaQ(a, x)`（级数 + Modified Lentz 连分式）
  - `regularizedIncompleteBeta(a, b, x)`（对称变换 + Modified Lentz 连分式）
- **6 大概率分布（PDF/PMF、CDF、PPF）**：
  - 正态分布：`normalPdf`, `normalCdf`, `normalPpf`
  - Student-t 分布：`studentTPdf`, `studentTCdf`, `studentTPpf`（有界 Newton-Halley 精确求逆）
  - 卡方分布：`chiSquarePdf`, `chiSquareCdf`, `chiSquarePpf`（Wilson-Hilferty + 有界 Newton）
  - F / Beta / Poisson 分布：`fDistCdf`, `betaPdf`, `betaCdf`, `poissonPmf`, `poissonCdf`
- **描述统计与假设检验**：
  - `DescriptiveStats` & `describeSample(xs)`（两遍补偿中心矩：`count`, `mean`, `variance`, `std_dev`, `skewness`, `kurtosis`, `min`, `max`, `sem`）
  - `TTestResult` & `oneSampleTTest(xs, mu0)` / `welchTwoSampleTTest(xs, ys)`（Welch-Satterthwaite 自由度、双侧 `p_value`、95% 置信区间与 Cohen's d 效应量）
  - `ChiSquareTestResult` & `chiSquareGoodnessOfFit(observed, expected)`
  - `CorrelationResult` & `pearsonCorrelationTest(xs, ys)`
- **简单 OLS & 多元 OLS / Ridge 线性回归**：
  - `SimpleRegressionResult` & `fitSimpleOls(xs, ys)`（`slope`, `intercept`, `r_squared`, `rmse`, `std_err_slope`, `p_value_slope`）
  - `MultiRegressionResult` & `fitRidgeRegression(x_rows, n_samples, n_features, y, ridge_lambda)` / `predictLinear(res, x_row)`（流式累积法方程 + 列主元高斯-约旦消元，不惩罚截距项，返回系数、`r_squared`、`adj_r_squared`、`rmse`）

```cp
import {
    describeSample,
    welchTwoSampleTTest,
    fitRidgeRegression,
    predictLinear,
} from "cap:math";

pub fn demoStatsAndRegression() {
    // 1. A/B 实验 Welch 双样本 t 检验
    const ctrl: [6]f64 = [10.2, 9.8, 10.5, 10.1, 9.9, 10.3];
    const treat: [6]f64 = [11.6, 12.1, 11.8, 12.0, 11.5, 11.9];
    const st = describeSample(treat[0..]) catch return;
    const tt = welchTwoSampleTTest(treat[0..], ctrl[0..]) catch return;
    * = st.mean;
    * = tt.p_value_two_tailed; // < 0.0001

    // 2. 多元 Ridge 线性回归 (y = 2.0 + 3.0*x0 - 1.5*x1)
    const x_rows: [10]f64 = [1.0, 0.0, 2.0, 1.0, 3.0, 2.0, 4.0, 1.0, 5.0, 3.0];
    const y: [5]f64 = [5.0, 6.5, 8.0, 12.5, 12.5];
    const model = fitRidgeRegression(x_rows[0..], 5, 2, y[0..], 0.0) catch return;
    const x_new: [2]f64 = [6.0, 2.0];
    const y_hat = predictLinear(model, x_new[0..]) catch 0.0; // ~17.0
    * = y_hat;
}
```

### 无上限外部工作区数值优化与两阶段单纯形线性规划套件 (`optimize.cp`)

面向工程最优化、运筹资源分配、模型参数拟合与黑盒调优的 100% 纯 CP 零动态堆分配优化套件。全部多维梯度、历史窗口与单纯形表均基于调用方传入的外部工作切片运行，无硬性维度玩具上限：

- **回调类型、错误集与结果模型**：
  - `OptScalarFn`, `OptObjectiveFn`, `OptGradientFn`
  - `OptError`（`InvalidDimension | InvalidBounds | InvalidTolerance | WorkBufferTooSmall | NonFiniteValue`）
  - `OptStatus`（`converged | max_iters_reached | infeasible | unbounded | stalled`）、`OptSummary`（`status`, `x_best`, `objective`, `grad_norm`, `iterations`, `func_evals`）
  - `LpStatus`（`optimal | infeasible | unbounded | max_iters_reached`）、`LpResult`（`status`, `objective`, `iterations`）
- **一维无导数极值与有界求根**：
  - `brentMinimize1d(f_fn, a, b, tol, max_iters): OptSummary ! OptError`（抛物线插值 + 黄金分割混合）
  - `brentRoot1d(f_fn, a, b, tol, max_iters): f64 ! OptError`（逆二次插值 + 割线 + 二分有界超线性求根）
- **两阶段单纯形线性规划（Two-Phase Simplex LP + Bland 防循环）**：
  - `lpTableauRequiredLen(num_vars, num_constraints): usize`
  - `solveLinearProgramSimplex(c, a_matrix, b_rhs, num_vars, num_constraints, *out_x, *tableau_work, *basis_work, max_iters): LpResult ! OptError`（求解 `min c^T x, s.t. A x <= b, x >= 0`，完整支持负右端项 Phase I 人工变量可行化、精准识别 `optimal` / `infeasible` / `unbounded`）
- **无约束多维数值优化器**：
  - `approxGradientCentral(f_fn, *x, eps, *out_grad)` / `armijoBacktrackingLineSearch(...)`
  - `cgWorkRequiredLen(dim)` & `minimizeConjugateGradient(f_fn, grad_fn, *x, tol_grad, max_iters, *work)`（Polak-Ribière+ 共轭梯度 + 自动重启）
  - `lbfgsWorkRequiredLen(dim, history_m)` & `minimizeLbfgs(f_fn, grad_fn, *x, history_m, tol_grad, max_iters, *work)`（Nocedal O(m·n) Two-Loop 拟牛顿法）
  - `nelderMeadWorkRequiredLen(dim)` & `minimizeNelderMead(f_fn, *x, init_step, tol_f, max_iters, *work)`（反射/扩张/收缩/压缩四态无导数单纯形）

```cp
import {
    brentMinimize1d,
    lpTableauRequiredLen,
    solveLinearProgramSimplex,
    lbfgsWorkRequiredLen,
    minimizeLbfgs,
} from "cap:math";

fn demoOptimize() {
    // 1. 标准生产资源分配 LP: min -3*x0 - 5*x1 s.t. x0<=4, 2*x1<=12, 3*x0+2*x1<=18, x>=0
    const c: [2]f64 = [-3.0, -5.0];
    const a: [6]f64 = [1.0, 0.0, 0.0, 2.0, 3.0, 2.0];
    const b: [3]f64 = [4.0, 12.0, 18.0];
    let x_opt: [2]f64 = [0.0; 2];
    let tab: [36]f64 = [0.0; 36]; // >= lpTableauRequiredLen(2, 3) = 36
    let basis: [3]usize = [0; 3];
    let x_s: []f64 = x_opt[0..];
    let tab_s: []f64 = tab[0..];
    let bas_s: []usize = basis[0..];
    const lp = solveLinearProgramSimplex(c[0..], a[0..], b[0..], 2, 3, *x_s, *tab_s, *bas_s, 100) catch return;
    // x_opt = [2.0, 6.0], objective = -36.0
    * = lp;
    * = lpTableauRequiredLen;
    * = lbfgsWorkRequiredLen;
    * = brentMinimize1d;
    * = minimizeLbfgs;
}
```

---

## 收敛子模块深度文档

- [anneal.md](math/anneal.md)
- [bessel.md](math/bessel.md)
- [bfgs.md](math/bfgs.md)
- [biquad.md](math/biquad.md)
- [cholesky.md](math/cholesky.md)
- [cusum.md](math/cusum.md)
- [decimal.md](math/decimal.md)
- [decimal_fp.md](math/decimal_fp.md)
- [dtw.md](math/dtw.md)
- [ekf.md](math/ekf.md)
- [fft.md](math/fft.md)
- [fir.md](math/fir.md)
- [ga.md](math/ga.md)
- [gamma.md](math/gamma.md)
- [hilbert.md](math/hilbert.md)
- [hmm.md](math/hmm.md)
- [holt.md](math/holt.md)
- [iir.md](math/iir.md)
- [kalman.md](math/kalman.md)
- [lu.md](math/lu.md)
- [markov.md](math/markov.md)
- [nelder.md](math/nelder.md)
- [newton.md](math/newton.md)
- [ode.md](math/ode.md)
- [particle.md](math/particle.md)
- [pca.md](math/pca.md)
- [pso.md](math/pso.md)
- [qr.md](math/qr.md)
- [quadrature.md](math/quadrature.md)
- [savitzky.md](math/savitzky.md)
- [simplex.md](math/simplex.md)
- [spline.md](math/spline.md)
- [svd.md](math/svd.md)
- [wavelet.md](math/wavelet.md)

---

## 零堆无上限密集矩阵引擎（对标 `dimforge/nalgebra` Apache-2.0 加固：`matrixDense.cp`）

面向科学计算、回归分析、卡尔曼滤波、PCA 与嵌入式机器学习的 100% 纯 CP 零动态堆分配、外部切片驱动密集矩阵核心（支持任意 $M \times N$ 维度，无硬编码小矩阵上限，含 NaN/Inf 前置防护与奇异/非对称拦截）：

- **矩阵视图与范数/基础算子**：`MatrixViewF64` / `MatrixMutF64` / `MatrixViewF32` / `MatrixMutF32`、`matrixIdentityF64`、`transposeMatrixF64`/`F32`、`matrixTraceF64`、`frobeniusNormF64`/`F32`、`frobeniusNormF32Simd`（`@dot` 加速）、`infNormF64`、`oneNormF64`、`matrixDiffFrobeniusF64`、`matrixPowerF64`。
- **CP 原生 SIMD `@dot` / `@Vector(8, f32)` 加速核心**：
  - `matmulF32Simd(a, b, alpha, beta, *c, *b_trans_work)`：转置 $B$ 后按行调用硬件 8-lane `@dot` 计算 $C = \alpha A B + \beta C$。
  - `matvecF32Simd`、`gramMatrixF32Simd`（利用对称性减半计算 $A^T A$）与 `outerProductAddF32Simd`（8-lane `@Vector(8, f32)` Rank-1 外积更新 $A \leftarrow A + \alpha x y^T$）。
- **三角求解与四大经典数值分解**：
  - **TRSV / TRSM 三角求解**：`solveLowerTriangular`、`solveUpperTriangular`、`solveTrsmLeftLower`、`solveTrsmLeftUpper`（支持单位对角与多右端矩阵求解）。
  - **部分主元 LU**：`luDecomposeInPlace`、`denseLuSolve`、`denseLuDeterminant` / `detFromLu`、`luInvert`。
  - **SPD Cholesky**：`choleskyDecomposeInPlace`（严格检测对称性与非正定返回 `NotPositiveDefinite`）、`denseCholeskySolve`、`denseCholeskyLogDet`。
  - **Householder QR & 超定最小二乘**：`qrDecomposeHouseholder`（支持任意 $M \times N$ 含宽矮 $M < N$ 与零列/秩亏矩阵）、`denseQrSolveLeastSquares`。
  - **对称 Jacobi 特征值 & One-Sided Jacobi 薄 SVD**：`eigenSymmetricJacobi`、`svdThin`、`svdThinAny`（自动支持 $M \ge N$ 与 $M < N$ 任意矩形矩阵）、`svdCondNumber`、`estimateConditionNumber`、`matrixRankF64`、`svdThinPinv`。

---

## 零堆无上限时间序列滤波、预测与异常检测引擎（`timeSeries.cp`）

面向监控告警、IoT 传感器追踪与量化/业务时序预测的 100% 纯 CP 零动态堆分配、外部切片驱动时序核心：

- **滑窗统计与季节平滑**：
  - `rollingMeanF64` / `rollingStdF64`（Welford 单遍稳定滑窗）、`ewmaSmoothF64`、`doubleExpSmoothHolt`。
  - `tsHoltWintersForecast`：支持任意周期 `period` 与加法/乘法季节性的三重指数平滑与多步预测。
  - `decomposeSeasonalAdditive`：居中移动平均（偶数周期 `2xP` / 奇数周期 `P`）提取 Trend + Seasonal + Residual，精确守恒 `trend[i] + seasonal[i] + residual[i] == y[i]`。
- **CP 原生 SIMD `@dot` 自相关 & AR(p) 预测**：
  - `computeAcfF32Simd`（硬件 `@dot` 加速滞后自相关）与 `computeAcfF64`。
  - `levinsonDurbinAr`（$O(p^2)$ Levinson-Durbin 递推解 Yule-Walker 输出 AR 系数、PACF 与噪声方差）+ `forecastArModel`。
- **生产级线性 Kalman 滤波器**：
  - `Kalman1dFilter`（2 状态恒速追踪器，Joseph 稳定协方差更新）。
  - `KalmanFilterN`（任意 $n$ 维状态 / $m$ 维观测空间滤波器，Joseph 稳定协方差更新 + 马氏距离新息门控）。
- **异常检测、变点检测与预测指标**：
  - `detectAnomaliesZScore`（滑窗 Z-Score）、`detectAnomaliesMad`（抗离群值污染 Robust MAD）、`CusumChangeDetector`（双边 Page-Hinkley/CUSUM 在线变点检测）、`evalForecastMetrics`（MAE/RMSE/MAPE/sMAPE/$R^2$）。
