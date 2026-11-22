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
| `lib/math/index.cp` | `cap:math` 与 `cap.math` 统一零冲突公开导出入口（共 297 符号，35 个实现子模块） |

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
