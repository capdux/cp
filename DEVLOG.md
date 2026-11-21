## 2026-11-22: 新建 `cap:bandit` 纯 CP 零分配多臂老虎机在线决策库：UCB1、Epsilon-Greedy（含衰减）、Marsaglia-Tsang Beta-Bernoulli Thompson Sampling、对数权重防溢出 EXP3 与累积遗憾计算（5/5 PASS，文档直写 `doc/lib/bandit.md`）
新增 cap:bandit（lib/bandit/{bandit,index}.cp）：零分配确定性 SplitMix64 + Marsaglia-Tsang Beta 采样器实现 UCB1/EpsGreedy/Thompson/EXP3，2000 轮收敛且 5000 步零溢出，cpc check 0 错误，cap test 6/6 PASS (674ms)

## 2026-11-22: 新建 `cap:dtw` 纯 CP 零分配动态时间规整（DTW）与时间序列对齐库：1D 滚动双行 O(m) DTW、Sakoe-Chiba 带宽约束、最优路径回溯、多维 DTW、LB_Keogh 下界包络与 Z-归一化（5/5 PASS，文档直写 `doc/lib/dtw.md`）
新增 cap:dtw（lib/dtw/{dtw,index}.cp）：零分配 O(m) 滚动双行与完整回溯 DTW、多维轨迹对齐及 48 组 LB_Keogh 严格下界校验，cpc check 0 错误，cap test 6/6 PASS (771ms)

## 2026-11-22: 新建 `cap:mcts` 纯 CP 零分配蒙特卡洛树搜索（MCTS / UCT & PUCT）决策规划库：左孩子右兄弟固定节点池、UCB1/PUCT 下探选择、先验展开、单人/双人零和回传与根节点策略统计（5/5 PASS，文档直写 `doc/lib/mcts.md`）
新增 cap:mcts（lib/mcts/{mcts,index}.cp）：零分配固定节点池 UCT/PUCT 搜索引擎，300 轮井字棋残局 100% 选中必胜与必防走法，cpc check 0 错误，cap test 6/6 PASS (838ms)

## 2026-11-22: 新建 `cap:bessel` 纯 CP 零分配柱贝塞尔与修正贝塞尔特殊函数库：Double-Double 106-bit 无损幂级数 + Hankel 渐近 + Miller 向下递推 + 指数缩放防溢出/下溢（5/5 PASS，文档直写 `doc/lib/bessel.md`）
新增 cap:bessel（lib/bessel/{bessel,index}.cp）：Double-Double 106-bit 幂级数 + Hankel 渐近 + Miller 向下递推与指数缩放版本，Wronskian 恒等式误差 < 1e-12，cpc check 0 错误，cap test 6/6 PASS (793ms)

## 2026-11-22: 新建 `cap:ekf` 纯 CP 零分配非线性状态估计库（EKF & UKF）：中心差分雅可比、Joseph 形式对称正定扩展卡尔曼滤波、新息 NIS 计算与 Merwe Sigma 点无迹卡尔曼滤波预测（5/5 PASS，文档直写 `doc/lib/ekf.md`）
新增 cap:ekf（lib/ekf/{ekf,index}.cp）：零分配 Joseph 形式 EKF 与 Merwe Sigma 点 UKF 预测，通过极坐标雷达 30 步闭环追踪与低噪正定性保持校验，cpc check 0 错误，cap test 6/6 PASS (805ms)

## 2026-11-22: 新建 `cap:markov` 纯 CP 零分配离散时间马尔可夫链（DTMC）分析与采样库：随机矩阵校验、快速幂、平稳分布幂迭代、吸收链基本矩阵 N=(I-Q)^-1、熵率与确定性轨迹采样（5/5 PASS，文档直写 `doc/lib/markov.md`）
新增 cap:markov（lib/markov/{markov,index}.cp）：零分配 DTMC 快速幂、平稳分布、赌徒破产吸收链 N=(I-Q)^-1 精确验证与 SplitMix64 轨迹采样，cpc check 0 错误，cap test 6/6 PASS (715ms)

## 2026-11-22: 新建 `cap:bfgs` 纯 CP 零分配拟牛顿法与梯度优化库：完整逆 Hessian BFGS、双循环有限内存 L-BFGS（O(mn) 内存）、Armijo/Wolfe 线搜索与中心差分梯度（5/5 PASS，文档直写 `doc/lib/bfgs.md`）
新增 cap:bfgs（lib/bfgs/{bfgs,index}.cp）：零分配完整 BFGS 与双循环 L-BFGS，40 步内收敛 Rosenbrock 与 16D 耦合二次/L2 逻辑回归目标，cpc check 0 错误，cap test 6/6 PASS (766ms)

## 2026-11-22: 新建 `cap:gamma` 纯 CP 零分配特殊数学函数库：Lanczos lgamma/tgamma、digamma、级数/Lentz 连分式正则化不完全 Gamma/Beta、无灾难抵消 erf/erfc 与对数阶乘/组合数（5/5 PASS，文档直写 `doc/lib/gamma.md`）
新增 cap:gamma（lib/gamma/{gamma,index}.cp）：9 项 Lanczos g=7、Bernoulli digamma、Lentz 连分式不完全 Gamma/Beta 与远尾高精度 erfc（相对误差 < 1e-13），cpc check 0 错误，cap test 6/6 PASS (716ms)

## 2026-11-22: 新建 `cap:ransac` 纯 CP 零分配确定性 RANSAC 鲁棒模型估计库：SplitMix64 采样与内点精修，支持 2D 直线（TLS）、2D 圆（Kåsa）、3D 平面（协方差最小特征向量）与 1D 线性回归（OLS）（5/5 PASS，文档直写 `doc/lib/ransac.md`）
新增 cap:ransac（lib/ransac/{ransac,index}.cp）：零分配 SplitMix64 确定性 RANSAC 鲁棒拟合，35%-40% 离群点下精确恢复 2D 线/圆、3D 平面与 1D 回归模型，cpc check 0 错误，cap test 6/6 PASS (812ms)

## 2026-11-22: 新建 `cap:nelder` 纯 CP 零分配无导数优化与一维求极值/求根库：Nelder-Mead 单纯形优化、一维黄金分割极小化与 Brent 超线性求根法（5/5 PASS，文档直写 `doc/lib/nelder.md`）
新增 cap:nelder（lib/nelder/{nelder,index}.cp）：零分配 Nelder-Mead 单纯形法（通过 Rosenbrock 与 4D 二次碗）、黄金分割极小化与 Brent 求根，cpc check 0 错误，cap test 6/6 PASS (750ms)

## 2026-11-22: 新建 `cap:quadrature` 纯 CP 零分配高精度数值积分库：Gauss-Legendre 5/10 点、复合 Simpson 1/3、无递归显式栈自适应 Gauss-Kronrod G7-K15 与 Romberg Richardson 外推积分（5/5 PASS，文档直写 `doc/lib/quadrature.md`）
新增 cap:quadrature（lib/quadrature/{quadrature,index}.cp）：零分配无递归显式栈自适应 QUADPACK G7-K15、Gauss-Legendre 与 Romberg 外推，通过弱奇异与窄尖峰积分验证，cpc check 0 错误，cap test 6/6 PASS (693ms)

## 2026-11-22: 新建 `cap:kmp` 纯 CP 零分配线性时间字符串匹配与回文算法库：KMP 前缀表/重叠匹配/最小周期、Z-Algorithm 与无哨兵 Manacher 最长回文子串（支持任意二进制字节串）（5/5 PASS，文档直写 `doc/lib/kmp.md`）
新增 cap:kmp（lib/kmp/{kmp,index}.cp）：零分配 O(n) KMP 前缀表与重叠匹配、Z 函数及无哨兵 Manacher 回文算法，支持任意二进制字节串，cpc check 0 错误，cap test 6/6 PASS (706ms)

## 2026-11-22: 新建 `cap:hmm` 纯 CP 零分配离散隐马尔可夫模型（HMM）库：对数域 Viterbi 最优路径解码、Rabiner 缩放 Forward-Backward 对数似然与后验分布及 Baum-Welch EM 参数重估（5/5 PASS，文档直写 `doc/lib/hmm.md`）
新增 cap:hmm（lib/hmm/{hmm,index}.cp）：零分配对数域 Viterbi 解码、Rabiner 缩放前向后向与 Baum-Welch EM，200 步长序列零下溢，cpc check 0 错误，cap test 6/6 PASS (761ms)

## 2026-11-22: 新建 `cap:suffix` 纯 CP 零分配后缀数组（Suffix Array）与 Kasai LCP 字符串索引库：倍增基数排序 O(n log n) SA、Kasai O(n) LCP、二分模式区间定位、不同子串计数与最长重复子串（5/5 PASS，文档直写 `doc/lib/suffix.md`）
新增 cap:suffix（lib/suffix/{suffix,index}.cp）：零分配倍增基数排序后缀数组与 Kasai O(n) LCP，支持任意二进制字节串，cpc check 0 错误，cap test 6/6 PASS (779ms)

## 2026-11-22: 新建 `cap:voronoi` 纯 CP 零分配 2D 有界 Voronoi 图与 Lloyd 松弛几何库：Sutherland-Hodgman 半平面裁剪、双缓冲 Voronoi 单元构建、面积/质心度量与 Lloyd 松弛迭代（5/5 PASS，文档直写 `doc/lib/voronoi.md`）
新增 cap:voronoi（lib/voronoi/{voronoi,index}.cp）：基于 ping-pong 双缓冲 Sutherland-Hodgman 半平面裁剪构建有界 Voronoi 单元与 Lloyd 松弛，12 点云面积守恒误差 < 1e-9，cpc check 0 错误，cap test 6/6 PASS (746ms)

## 2026-11-22: 新建 `cap:bipartite` 纯 CP 零分配二分图算法库：BFS 二染色二分性判定、Hopcroft-Karp O(E√V) 最大基数匹配与 Kőnig 定理最小点覆盖/最大独立集提取（5/5 PASS，文档直写 `doc/lib/bipartite.md`）
新增 cap:bipartite（lib/bipartite/{bipartite,index}.cp）：零分配 BFS 二染色、Hopcroft-Karp O(E√V) 最大基数匹配与 Kőnig 最小点覆盖，通过 50x50 稀疏图联合校验，cpc check 0 错误，cap test 6/6 PASS (824ms)

## 2026-11-22: 新建 `cap:delaunay` 纯 CP 零分配 2D Delaunay 三角剖分库：Bowyer-Watson 增量剖分、无除法外接圆行列式谓词、CCW 绕序保证、空外接圆准则检验与三角形点定位（5/5 PASS，文档直写 `doc/lib/delaunay.md`）
新增 cap:delaunay（lib/delaunay/{delaunay,index}.cp）：零分配 Bowyer-Watson 增量三角剖分含 3x3 无除法外接圆行列式与空外接圆准则校验，cpc check 0 错误，cap test 6/6 PASS (700ms)

## 2026-11-22: 新建 `cap:treap` 纯 CP 零分配随机平衡树与顺序统计 Treap 库：固定节点池与空闲链表复用、SplitMix64 优先级、O(log n) kth/rank/lowerBound/upperBound/rangeCount 与不变量校验（5/5 PASS，文档直写 `doc/lib/treap.md`）
新增 cap:treap（lib/treap/{treap,index}.cp）：零分配固定节点池与空闲链表实现 SplitMix64 随机平衡 Treap，支持 O(log n) kth/rank/rangeCount 与 BST+堆序不变量校验，cpc check 0 错误，cap test 6/6 PASS (852ms)

## 2026-11-22: 新建 `cap:knn` 纯 CP 零分配 K-最近邻（KNN）分类与回归库：欧氏/曼哈顿/切比雪夫/余弦四种距离、原地大顶堆 Top-K 搜索、均匀/反距离加权分类与回归及留一法交叉验证（5/5 PASS，文档直写 `doc/lib/knn.md`）
新增 cap:knn（lib/knn/{knn,index}.cp）：零分配大顶堆 Top-K 最近邻搜索、均匀/反距离加权分类与回归及留一法交叉验证，cpc check 0 错误，cap test 6/6 PASS (691ms)

## 2026-11-22: 新建 `cap:pca` 纯 CP 零分配主成分分析（PCA）与实对称矩阵 Jacobi 特征分解库：循环 Jacobi 特征分解、无偏协方差 PCA 拟合、降维投影、逆变换重构与均方误差检验（5/5 PASS，文档直写 `doc/lib/pca.md`）
新增 cap:pca（lib/pca/{pca,index}.cp）：基于零分配循环 Jacobi 特征分解实现 PCA 拟合/投影/重构与丢弃特征值重构误差恒等式验证，cpc check 0 错误，cap test 6/6 PASS (634ms)

## 2026-11-22: 新建 `cap:svd` 纯 CP 零分配单边 Jacobi 奇异值分解与广义逆库：经济型 SVD（含零奇异值正交补全）、截断奇异值最小范数最小二乘与 Moore-Penrose 伪逆（5/5 PASS，文档直写 `doc/lib/svd.md`）
新增 cap:svd（lib/svd/{svd,index}.cp）：零分配单边 Jacobi SVD 含二次 MGS 零奇异值正交补全与 Penrose 四大公理验证，cpc check 0 错误，cap test 6/6 PASS (790ms)

## 2026-11-22: 新建 `cap:qr` 纯 CP 零分配 Householder QR 分解与线性最小二乘库：原地 Householder 反射分解、Q^T 乘法、经济型 Q/R 构造、超定最小二乘求解与极端量级数值稳定防护（5/5 PASS，文档直写 `doc/lib/qr.md`）
新增 cap:qr（lib/qr/{qr,index}.cp）：基于 Householder 反射实现高数值稳定性 QR 分解与超定线性最小二乘，通过 Hilbert 病态矩阵与 1e±100 量级缩放测试，cpc check 0 错误，cap test 6/6 PASS (842ms)

## 2026-11-22: 新建 `cap:lu` 纯 CP 零分配部分选主元 LU 分解与三对角求解库：Doolittle PA=LU 分解、多右端项求解、行列式、矩阵求逆、残差检验与 O(n) Thomas 三对角追赶法（5/5 PASS，文档直写 `doc/lib/lu.md`）
新增 cap:lu（lib/lu/{lu,index}.cp）：原地部分选主元 Doolittle LU 分解、多右端项复用求解、行列式与逆矩阵计算及 100 维 Thomas 三对角求解，cpc check 0 错误，cap test 6/6 PASS (624ms)

## 2026-11-22: 新建 `cap:simplex` 纯 CP 零分配两阶段线性规划单纯形求解器库：Bland 防退化循环规则、混合 <=/>=/= 约束、Phase-I 人工变量、无界/不可行检出与原/对偶影子价格提取（5/5 PASS，文档直写 `doc/lib/simplex.md`）
新增 cap:simplex（lib/simplex/{simplex,index}.cp）：基于调用方 tableau/basis 缓冲实现两阶段单纯形法与 Bland 防退化规则，通过 Beale 1955 退化基准与强对偶验证，cpc check 0 错误，cap test 7/7 PASS (761ms)

## 2026-11-22: 新建 `cap:mst` 纯 CP 零分配最小生成树与生成森林算法库：路径压缩并查集、Kruskal 最小/最大生成树、稠密图 O(V²) Prim 生成森林与生成树无环连通校验（5/5 PASS，文档直写 `doc/lib/mst.md`）
新增 cap:mst（lib/mst/{mst,index}.cp）：基于零分配路径压缩并查集实现 Kruskal 最小/最大生成树、稠密矩阵 Prim 最小生成森林与无环连通校验，cpc check 0 错误，cap test 6/6 PASS (699ms)

## 2026-11-22: 新建 `cap:dijkstra` 纯 CP 零分配图最短路径算法库：索引二叉最小堆 Dijkstra、路径还原、Bellman-Ford 负权边与负环检测及 Floyd-Warshall 全源最短路径（5/5 PASS，文档直写 `doc/lib/dijkstra.md`）
新增 cap:dijkstra（lib/dijkstra/{dijkstra,index}.cp）：内置零分配索引二叉最小堆实现 O((V+E)logV) Dijkstra、防环路径还原、Bellman-Ford 负环检测与 Floyd-Warshall 全源最短路，cpc check 0 错误，cap test 6/6 PASS (929ms)

## 2026-11-22: 新建 `cap:hungarian` 纯 CP 零分配 O(n³) 势能对偶匈牙利线性指派库：支持 i64/f64 最小权与最大权二分图最优匹配及任意矩形矩阵零拷贝自动转置（5/5 PASS，文档直写 `doc/lib/hungarian.md`）
新增 cap:hungarian（lib/hungarian/{hungarian,index}.cp）：基于 Jonker-Volgenant/Kuhn-Munkres u/v 势能对偶实现零分配 O(n³) 线性指派与强对偶互补松弛验证，cpc check 0 错误，cap test 6/6 PASS (769ms)

## 2026-11-22: 新建 `cap:octree` 纯 CP 零分配 3D 空间八叉树索引库：固定池八叉树、AABB/球体范围查询、下界剪枝最近邻搜索与 Slab 法射线-AABB 相交查询（5/5 PASS，文档直写 `doc/lib/octree.md`）
新增 cap:octree（lib/octree/{octree,index}.cp）：基于静态节点/条目池实现 3D 八叉树自动分裂、盒/球范围查询、下界剪枝最近邻与 Slab 射线相交，cpc check 0 错误，cap test 6/6 PASS (758ms)

## 2026-11-22: 新建 `cap:rmq` 纯 CP 零分配区间查询与滑动窗口算法库：O(1) Sparse Table（Min/Max/ArgMin/GCD）、O(1) 欧拉序树上 LCA 与 O(n) 单调队列滑动窗口极值（5/5 PASS，文档直写 `doc/lib/rmq.md`）
新增 cap:rmq（lib/rmq/{rmq,index}.cp）：基于调用方缓冲实现 O(1) Sparse Table 区间最值/首次下标/GCD、O(1) 欧拉序 LCA 与 O(n) 单调队列滑动窗口，cpc check 0 错误，cap test 6/6 PASS (717ms)

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

