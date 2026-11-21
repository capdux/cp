## 2026-11-22: 新建 `cap:scc` 纯 CP 零分配有向图强连通分量与拓扑算法库：CSR 图、纯迭代无递归 Tarjan SCC、Kahn 拓扑排序/环检测、DAG 最长路径与线性时间 2-SAT 求解器（5/5 PASS，文档直写 `doc/lib/scc.md`）
新增 cap:scc（lib/scc/{scc,index}.cp）：基于 CSR 与显式调用栈实现无递归 Tarjan SCC、Kahn 拓扑排序与 2-SAT，全路径零堆分配，cpc check 0 错误，cap test 6/6 PASS (816ms)

## 2026-11-22: 新建 `cap:maxflow` 纯 CP 零分配网络流算法库：CSR 残量图、Dinic 最大流（BFS 分层 + 当前弧优化）、最小割可达性判定与 SPFA 最小费用最大流（MCMF）（5/5 PASS，文档直写 `doc/lib/maxflow.md`）
新增 cap:maxflow（lib/maxflow/{maxflow,index}.cp）：采用静态邻接表残量图 + 当前弧优化 Dinic 算法与 SPFA MCMF，全路径零堆分配，cpc check 0 错误，cap test 6/6 PASS (688ms)

## 2026-11-22: 新建 `cap:pagerank` CSR directed graph PageRank (dangling-node), Personalized PageRank & Kleinberg HITS（5/5 PASS，文档直写 `doc/lib/pagerank.md`）
新增 cap:pagerank 纯 CP 零分配图中心性算法库（悬挂节点重分配、Kahan 求和与 L2 归一化 HITS），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:kmeans` Deterministic K-Means++ clustering (arbitrary D), empty-cluster recovery & Silhouette（5/5 PASS，文档直写 `doc/lib/kmeans.md`）
新增 cap:kmeans 纯 CP 零分配聚类分析库（SplitMix64 + D^2 加权初始化、64 维分块任意高维 Lloyd 迭代与轮廓系数），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:polyfit` OLS/Ridge polynomial regression (streaming Givens QR), Horner calculus & root-finding（5/5 PASS，文档直写 `doc/lib/polyfit.md`）
新增 cap:polyfit 纯 CP 零分配数值拟合库（坐标归一化 + 栈上流式 Givens QR、L2 岭回归与二分/牛顿求根），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:gjk` 2D GJK convex collision, Minkowski support, SAT minimum translation vector & circle-poly（5/5 PASS，文档直写 `doc/lib/gjk.md`）
新增 cap:gjk 纯 CP 零分配物理碰撞检测库（2D 单纯形演化、SAT 分离轴 MTV 推离验证与 Voronoi 最近顶点圆碰撞），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:wavelet` Discrete Wavelet Transform: Haar, Daubechies-4 (db2) & JPEG 2000 lossless CDF 5/3（5/5 PASS，文档直写 `doc/lib/wavelet.md`）
新增 cap:wavelet 纯 CP 零分配多分辨率小波变换库（能量守恒 Haar/db4、逐比特无损 CDF 5/3 整数提升与软/硬阈值去噪），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:ode` Numerical ODE solvers: RK2, RK4, adaptive Dormand-Prince RK4(5) & Velocity Verlet（5/5 PASS，文档直写 `doc/lib/ode.md`）
新增 cap:ode 纯 CP 零分配常微分方程求解库（完整 7 级 Dormand-Prince Butcher 表自适应步长与能量守恒辛积分），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:perlin` Ken Perlin (2002) 1D/2D/3D gradient noise, fBm, Ridged multifractal & Turbulence（5/5 PASS，文档直写 `doc/lib/perlin.md`）
新增 cap:perlin 纯 CP 零分配程序化噪声库（quintic 平滑、classic/seeded 512B 置换表与多倍频分形叠加），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:cholesky` SPD Cholesky (LL^T) & square-root-free LDL^T decomposition, solver, logDet & inverse（5/5 PASS，文档直写 `doc/lib/cholesky.md`）
新增 cap:cholesky 纯 CP 零分配对称矩阵分解库（L L^T / L D L^T 求解残差 < 1e-12 与对称正定求逆），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:quat` 3D unit quaternions: Hamilton mul, Euler ZYX, Shepperd Mat3, SLERP & fromToRotation（5/5 PASS，文档直写 `doc/lib/quat.md`）
新增 cap:quat 纯 CP 零分配 3D 姿态与旋转库（全 4 分支 Shepperd 矩阵转换、万向锁处理与最短弧 SLERP），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:astar` Zero-alloc 2D grid A* pathfinding (4/8-way, anti-corner-cut min-heap) & Bresenham LOS（5/5 PASS，文档直写 `doc/lib/astar.md`）
新增 cap:astar 纯 CP 零分配游戏与机器人寻路库（原地降键二叉最小堆、Manhattan/Octile 启发式与整数视线检测），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:earcut` 2D simple polygon ear-clipping triangulation, signed area & point-in-polygon（5/5 PASS，文档直写 `doc/lib/earcut.md`）
新增 cap:earcut 纯 CP 零分配计算几何剖分库（CCW/CW 自动归一化、凹多边形/五角星耳切剖分面积守恒与射线法），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:spline` Natural/Clamped cubic splines (Thomas O(n)), monotone PCHIP & Catmull-Rom interpolation（5/5 PASS，文档直写 `doc/lib/spline.md`）
新增 cap:spline 纯 CP 零分配数值插值库（O(n) Thomas 三对角求解、精确解析积分与单调保形无过冲 PCHIP），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:bezier` 2D Quadratic/Cubic Bezier: De Casteljau, curvature, Gauss-Legendre arc length & tight AABB（5/5 PASS，文档直写 `doc/lib/bezier.md`）
新增 cap:bezier 纯 CP 零分配计算几何曲线库（升阶、细分、复合 5 点 Gauss-Legendre 弧长与精确极值包围盒），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:fir` DSP Windowed-Sinc FIR filter, Hann/Hamming/Blackman windows & RBJ Biquad IIR（5/5 PASS，文档直写 `doc/lib/fir.md`）
新增 cap:fir 纯 CP 零分配数字信号处理库（环形延迟线 FIR、低通系数设计与 Direct Form II Transposed Biquad 解析频响），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:dct` Orthonormal 1D & 2D 8x8 DCT-II/IDCT-III, JPEG ZigZag scan & IJG quantization（5/5 PASS，文档直写 `doc/lib/dct.md`）
新增 cap:dct 纯 CP 零分配图像与信号变换库（能量守恒 1D DCT、可分离 8×8 块变换与 IJG 质量缩放量化），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:kalman` Discrete 1D & 2D constant-velocity Kalman filter (Joseph form) & Alpha-Beta tracker（5/5 PASS，文档直写 `doc/lib/kalman.md`）
新增 cap:kalman 纯 CP 零分配传感器融合与状态估计库（1D/2D 恒速模型、Joseph 正定协方差更新与 Mahalanobis 野值门控），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:pid` Discrete PID controller with derivative-on-measurement, anti-windup, LPF & bumpless transfer（5/5 PASS，文档直写 `doc/lib/pid.md`）
新增 cap:pid 纯 CP 零分配闭环控制库（含 Ziegler-Nichols 整定、斜率限幅与条件积分 Anti-Windup），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:gf256` Galois Field GF(2^8) (AES/RS) arithmetic, const-time mul, Lagrange & Gauss-Jordan（5/5 PASS，文档直写 `doc/lib/gf256.md`）
新增 cap:gf256 纯 CP 零分配有限域库（全 256×256 查表与无分支常量时间一致、RAID-6 矩阵求逆恢复与 Shamir 门限插值），通过 cpc check 与 5/5 cap test



## 2026-11-22: 新建 `cap:fnv` IETF FNV-1/FNV-1a (32/64/128-bit) streaming hashers + DJB2/SDBM/Jenkins OAAT（5/5 PASS，文档直写 `doc/lib/fnv.md`）
新增 cap:fnv 纯 CP 零分配经典非加密哈希套件（覆盖 IETF 32/64/128-bit 官方测试向量与流式分块增量更新），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:roman` Strict canonical Roman numerals (1..3999) & Bijective Base-26 spreadsheet column codec（5/5 PASS，文档直写 `doc/lib/roman.md`）
新增 cap:roman 纯 CP 零分配数字与列号转换库（1..3999 穷举双射、严格/宽松模式与 Bijective Base-26 A..XFD），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:minhash` Andrei Broder MinHash Jaccard estimator, union merge, cardinality & LSH band hashing（5/5 PASS，文档直写 `doc/lib/minhash.md`）
新增 cap:minhash 纯 CP 零分配集合相似度估计库（SplitMix64 盐哈希族、并集签名合并与 32×4 LSH 分带检索），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:simhash` Charikar 64-bit SimHash near-duplicate fingerprinting, shingles & 4x16-bit LSH（5/5 PASS，文档直写 `doc/lib/simhash.md`）
新增 cap:simhash 纯 CP 零分配近似重复文档指纹库（栈上 64 维权重向量、中英文 shingle 与 4 段鸽巢 LSH），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:porter` Martin Porter (1980) English stemming algorithm (Steps 1a-5b) zero-alloc implementation（5/5 PASS，文档直写 `doc/lib/porter.md`）
新增 cap:porter 纯 CP 零分配自然语言处理词干库（完整实现 1980 论文全部 5 步规则并通过 60+ 向量验证），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:olc` Google Open Location Code / Plus Codes Base-20 geo-grid encoder, decoder & recovery（5/5 PASS，文档直写 `doc/lib/olc.md`）
新增 cap:olc 纯 CP 零分配全球网格地理编码库（高精度整数网格防浮点漂移、2..15 位编码与跨象限近邻恢复），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:lfu` Zero-alloc LFU cache (LRU tie-break) & 4-bit W-TinyLFU Count-Min Sketch admission filter（5/5 PASS，文档直写 `doc/lib/lfu.md`）
新增 cap:lfu 纯 CP 零分配高频缓存与准入控制库（最低频次+LRU 破平局淘汰与 4-bit 饱和周期减半 Sketch），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:soundex` Phonetic indexing & fuzzy name matching: NARA Soundex, Metaphone, NYSIIS & Cologne（5/5 PASS，文档直写 `doc/lib/soundex.md`）
新增 cap:soundex 纯 CP 零分配语音编码与模糊姓名匹配库（覆盖全部 4 种经典标准向量），通过 cpc check 与 6/6 cap test

