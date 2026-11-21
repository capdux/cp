# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-22] - 新建 `cap:dijkstra` 纯 CP 零分配图最短路径算法库：索引二叉最小堆 Dijkstra、路径还原、Bellman-Ford 负权边与负环检测及 Floyd-Warshall 全源最短路径（5/5 PASS，自动动态挂载 `cap.dijkstra`）
新增纯 CP 零分配图最短路径算法库 cap:dijkstra（索引堆 Dijkstra、路径还原、Bellman-Ford、Floyd-Warshall），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:hungarian` 纯 CP 零分配 O(n³) 势能对偶匈牙利线性指派库：支持 i64/f64 最小权与最大权二分图最优匹配及任意矩形矩阵零拷贝自动转置（5/5 PASS，自动动态挂载 `cap.hungarian`）
新增纯 CP 零分配 O(n³) 匈牙利线性指派库 cap:hungarian（i64/f64 最小与最大权匹配、矩形矩阵自动转置），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:octree` 纯 CP 零分配 3D 空间八叉树索引库：固定池八叉树、AABB/球体范围查询、下界剪枝最近邻搜索与 Slab 法射线-AABB 相交查询（5/5 PASS，自动动态挂载 `cap.octree`）
新增纯 CP 零分配 3D 空间八叉树索引库 cap:octree（固定池八叉树、AABB/球体查询、最近邻剪枝、Slab 射线查询），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:rmq` 纯 CP 零分配区间查询与滑动窗口算法库：O(1) Sparse Table（Min/Max/ArgMin/GCD）、O(1) 欧拉序树上 LCA 与 O(n) 单调队列滑动窗口极值（5/5 PASS，自动动态挂载 `cap.rmq`）
新增纯 CP 零分配区间查询与滑动窗口库 cap:rmq（Sparse Table RMQ/ArgMin/GCD、欧拉序 LCA、单调队列滑动窗口），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:scc` 纯 CP 零分配有向图强连通分量与拓扑算法库：CSR 图、纯迭代无递归 Tarjan SCC、Kahn 拓扑排序/环检测、DAG 最长路径与线性时间 2-SAT 求解器（5/5 PASS，自动动态挂载 `cap.scc`）
新增纯 CP 零分配有向图强连通分量与 2-SAT 库 cap:scc（迭代 Tarjan SCC、Kahn 拓扑排序、DAG 最长路、2-SAT），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:maxflow` 纯 CP 零分配网络流算法库：CSR 残量图、Dinic 最大流（BFS 分层 + 当前弧优化）、最小割可达性判定与 SPFA 最小费用最大流（MCMF）（5/5 PASS，自动动态挂载 `cap.maxflow`）
新增纯 CP 零分配网络流算法库 cap:maxflow（Dinic 最大流、最小割、SPFA 最小费用最大流），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:pagerank` CSR directed graph PageRank (dangling-node), Personalized PageRank & Kleinberg HITS（5/5 PASS，自动动态挂载 `cap.pagerank`）
新增纯 CP 零分配标准库 cap:pagerank（CSR 有向图 PageRank、Personalized PageRank 与 Kleinberg HITS）与 6/6 深度测试

## [2026-11-22] - 新建 `cap:kmeans` Deterministic K-Means++ clustering (arbitrary D), empty-cluster recovery & Silhouette（5/5 PASS，自动动态挂载 `cap.kmeans`）
新增纯 CP 零分配标准库 cap:kmeans（任意维度确定性 K-Means++ 聚类、空簇恢复与 Silhouette 轮廓系数）与 6/6 深度测试

## [2026-11-22] - 新建 `cap:polyfit` OLS/Ridge polynomial regression (streaming Givens QR), Horner calculus & root-finding（5/5 PASS，自动动态挂载 `cap.polyfit`）
新增纯 CP 零分配标准库 cap:polyfit（流式 Givens QR 多项式拟合、岭回归、Horner 微积分与实根求解）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:gjk` 2D GJK convex collision, Minkowski support, SAT minimum translation vector & circle-poly（5/5 PASS，自动动态挂载 `cap.gjk`）
新增纯 CP 零分配标准库 cap:gjk（2D GJK 凸多边形碰撞检测、SAT 最小穿透向量 MTV 与圆-多边形碰撞）与 6/6 深度测试

## [2026-11-22] - 新建 `cap:wavelet` Discrete Wavelet Transform: Haar, Daubechies-4 (db2) & JPEG 2000 lossless CDF 5/3（5/5 PASS，自动动态挂载 `cap.wavelet`）
新增纯 CP 零分配标准库 cap:wavelet（正交 Haar、Daubechies-4 db2 与 JPEG 2000 无损整数提升 CDF 5/3 小波）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:ode` Numerical ODE solvers: RK2, RK4, adaptive Dormand-Prince RK4(5) & Velocity Verlet（5/5 PASS，自动动态挂载 `cap.ode`）
新增纯 CP 零分配标准库 cap:ode（经典 RK4、自适应 Dormand-Prince RK4(5) 与能量守恒 Velocity Verlet 辛积分器）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:perlin` Ken Perlin (2002) 1D/2D/3D gradient noise, fBm, Ridged multifractal & Turbulence（5/5 PASS，自动动态挂载 `cap.perlin`）
新增纯 CP 零分配标准库 cap:perlin（Ken Perlin 2002 1D/2D/3D 梯度噪声、fBm、Ridged 与 Turbulence）与 6/6 深度测试

## [2026-11-22] - 新建 `cap:cholesky` SPD Cholesky (LL^T) & square-root-free LDL^T decomposition, solver, logDet & inverse（5/5 PASS，自动动态挂载 `cap.cholesky`）
新增纯 CP 零分配标准库 cap:cholesky（SPD Cholesky LL^T 与无开方 LDL^T 分解、求解、对数行列式与求逆）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:quat` 3D unit quaternions: Hamilton mul, Euler ZYX, Shepperd Mat3, SLERP & fromToRotation（5/5 PASS，自动动态挂载 `cap.quat`）
新增纯 CP 零分配标准库 cap:quat（3D 单位四元数、欧拉角 ZYX、Shepperd 3×3 矩阵与最短弧 SLERP）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:astar` Zero-alloc 2D grid A* pathfinding (4/8-way, anti-corner-cut min-heap) & Bresenham LOS（5/5 PASS，自动动态挂载 `cap.astar`）
新增纯 CP 零分配标准库 cap:astar（2D 网格 A* 最短路径搜索、8 邻域防切角与 Bresenham 视线检测）与 6/6 深度测试

## [2026-11-22] - 新建 `cap:earcut` 2D simple polygon ear-clipping triangulation, signed area & point-in-polygon（5/5 PASS，自动动态挂载 `cap.earcut`）
新增纯 CP 零分配标准库 cap:earcut（2D 简单多边形耳切法三角剖分、有向面积与点在多边形内判定）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:spline` Natural/Clamped cubic splines (Thomas O(n)), monotone PCHIP & Catmull-Rom interpolation（5/5 PASS，自动动态挂载 `cap.spline`）
新增纯 CP 零分配标准库 cap:spline（Natural/Clamped 三次样条、Fritsch-Carlson 单调保形 PCHIP 与 Catmull-Rom）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:bezier` 2D Quadratic/Cubic Bezier: De Casteljau, curvature, Gauss-Legendre arc length & tight AABB（5/5 PASS，自动动态挂载 `cap.bezier`）
新增纯 CP 零分配标准库 cap:bezier（2D 二次/三次贝塞尔曲线、De Casteljau、曲率、弧长积分与解析 AABB）与 6/6 深度测试

## [2026-11-22] - 新建 `cap:fir` DSP Windowed-Sinc FIR filter, Hann/Hamming/Blackman windows & RBJ Biquad IIR（5/5 PASS，自动动态挂载 `cap.fir`）
新增纯 CP 零分配标准库 cap:fir（Windowed-Sinc FIR、Hann/Hamming/Blackman 窗函数与 RBJ Biquad IIR 滤波器）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:dct` Orthonormal 1D & 2D 8x8 DCT-II/IDCT-III, JPEG ZigZag scan & IJG quantization（5/5 PASS，自动动态挂载 `cap.dct`）
新增纯 CP 零分配标准库 cap:dct（正交 1D/2D 8×8 DCT-II/IDCT-III、JPEG ZigZag 与 IJG 量化）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:kalman` Discrete 1D & 2D constant-velocity Kalman filter (Joseph form) & Alpha-Beta tracker（5/5 PASS，自动动态挂载 `cap.kalman`）
新增纯 CP 零分配标准库 cap:kalman（离散 1D/2D 卡尔曼滤波器、Joseph 稳定协方差与 Alpha-Beta 滤波）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:pid` Discrete PID controller with derivative-on-measurement, anti-windup, LPF & bumpless transfer（5/5 PASS，自动动态挂载 `cap.pid`）
新增纯 CP 零分配标准库 cap:pid（工业离散 PID 控制器：微分先行、抗积分饱和、一阶低通与无扰切换）与 6/6 深度测试

## [2026-11-22] - 新建 `cap:gf256` Galois Field GF(2^8) (AES/RS) arithmetic, const-time mul, Lagrange & Gauss-Jordan（5/5 PASS，自动动态挂载 `cap.gf256`）
新增纯 CP 零分配标准库 cap:gf256（伽罗瓦域 GF(2^8) AES/RS 算术、常量时间乘法、Lagrange 插值与矩阵求逆）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:fnv` IETF FNV-1/FNV-1a (32/64/128-bit) streaming hashers + DJB2/SDBM/Jenkins OAAT（5/5 PASS，自动动态挂载 `cap.fnv`）
新增纯 CP 零分配标准库 cap:fnv（IETF FNV-1/FNV-1a 32/64/128-bit 流式哈希与 DJB2/SDBM/Jenkins OAAT）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:roman` Strict canonical Roman numerals (1..3999) & Bijective Base-26 spreadsheet column codec（5/5 PASS，自动动态挂载 `cap.roman`）
新增纯 CP 零分配标准库 cap:roman（严格规范罗马数字 1..3999 与电子表格 Base-26 列号编解码）与 6/6 深度测试

## [2026-11-22] - 新建 `cap:minhash` Andrei Broder MinHash Jaccard estimator, union merge, cardinality & LSH band hashing（5/5 PASS，自动动态挂载 `cap.minhash`）
新增纯 CP 零分配标准库 cap:minhash（Andrei Broder MinHash Jaccard 估计、签名单调合并与 LSH 分带哈希）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:simhash` Charikar 64-bit SimHash near-duplicate fingerprinting, shingles & 4x16-bit LSH（5/5 PASS，自动动态挂载 `cap.simhash`）
新增纯 CP 零分配标准库 cap:simhash（64-bit SimHash 近似去重指纹、k-gram 切片与 4×16-bit LSH 分段键）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:porter` Martin Porter (1980) English stemming algorithm (Steps 1a-5b) zero-alloc implementation（5/5 PASS，自动动态挂载 `cap.porter`）
新增纯 CP 零分配标准库 cap:porter（Martin Porter 1980 英文词干提取算法 Steps 1a–5b）与 6/6 深度测试

## [2026-11-22] - 新建 `cap:olc` Google Open Location Code / Plus Codes Base-20 geo-grid encoder, decoder & recovery（5/5 PASS，自动动态挂载 `cap.olc`）
新增纯 CP 零分配标准库 cap:olc（Google Open Location Code / Plus Codes 编解码、缩短与近邻恢复）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:lfu` Zero-alloc LFU cache (LRU tie-break) & 4-bit W-TinyLFU Count-Min Sketch admission filter（5/5 PASS，自动动态挂载 `cap.lfu`）
新增纯 CP 零分配标准库 cap:lfu（固定容量 LFU 缓存与 4-bit W-TinyLFU Count-Min Sketch 准入过滤器）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:soundex` Phonetic indexing & fuzzy name matching: NARA Soundex, Metaphone, NYSIIS & Cologne（5/5 PASS，自动动态挂载 `cap.soundex`）
新增纯 CP 零分配标准库 cap:soundex（NARA Soundex、Metaphone、NYSIIS 与 Cologne Phonetics 语音索引套件）与 6/6 深度测试

