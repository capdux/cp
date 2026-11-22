# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-22] - 新建 `cap:bloburl` 吸收自 Bun runtime/webcore/ObjectURLRegistry.cp 的纯 CP 零分配 W3C blob: Object URL 解析、RFC 4122 UUID v4 生成与固定池注册表库（5/5 PASS，自动动态挂载 `cap.bloburl`）
吸收 Bun 上游 ObjectURLRegistry.cp 为纯 CP 零分配库 cap:bloburl（blob: URL 解析与固定池注册表），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:textdecode` 吸收自 Bun runtime/webcore/TextDecoder.cp 的纯 CP 零分配 WHATWG 流式文本解码器：UTF-8/UTF-16LE/UTF-16BE/Latin-1 跨块重组、BOM 探测剥离与 fatal/U+FFFD 替换模式（5/5 PASS，自动动态挂载 `cap.textdecode`）
吸收 Bun 上游 TextDecoder.cp 为纯 CP 零分配库 cap:textdecode（WHATWG 流式文本解码器），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:cssmodules` 吸收自 Bun css/css_modules.cp 的纯 CP 零分配 CSS Modules 命名模板、Wyhash+Base64 确定性哈希、作用域类名编译与 composes 跨文件依赖解析库（5/5 PASS，自动动态挂载 `cap.cssmodules`）
吸收 Bun 上游 css/css_modules.cp 为纯 CP 零分配库 cap:cssmodules（CSS Modules 命名模板、哈希与 composes 解析），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:mediaquery` 吸收自 Bun css/media_query.cp 的纯 CP 零分配 W3C CSS Media Queries Level 4 库：长度/分辨率/宽高比单位换算、单/双边区间范围、媒体特性求值与规范化（5/5 PASS，自动动态挂载 `cap.mediaquery`）
吸收 Bun 上游 css/media_query.cp 为纯 CP 零分配库 cap:mediaquery（CSS Media Queries Level 4 解析/求值/规范化），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:textstream` 吸收自 Bun runtime/webcore/TextEncoderStreamEncoder.cp 的纯 CP 零分配 WHATWG 流式 UTF-8 编码器：跨分块 UTF-16 代理对拼接、孤立代理 U+FFFD 替换、Latin-1 转码与小缓冲原子保护（5/5 PASS，自动动态挂载 `cap.textstream`）
吸收 Bun 上游 TextEncoderStreamEncoder.cp 为纯 CP 零分配库 cap:textstream（WHATWG 流式 UTF-8 编码器），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:h2stream` 吸收自 Bun http/h2_client/Stream.cp 的纯 CP 零分配 HTTP/2（RFC 9113）流状态机与双向流控窗口库：半关闭生命周期、14 种错误码、RST_STREAM/WINDOW_UPDATE 帧编码与自动补窗（5/5 PASS，自动动态挂载 `cap.h2stream`）
吸收 Bun 上游 http/h2_client/Stream.cp 为纯 CP 零分配库 cap:h2stream（RFC 9113 HTTP/2 流状态机与流控窗口），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:qpack` 吸收自 Bun uws_sys/quic/Header.cp 与 http/h3_client/encode.cp 的纯 CP 零分配 HTTP/3 QPACK（RFC 9204）编解码库：静态表分类、多前缀整数编解码、请求伪头部编排与 Field Section 零拷贝往返（5/5 PASS，自动动态挂载 `cap.qpack`）
吸收 Bun 上游 uws_sys/quic/Header.cp & h3_client/encode.cp 为纯 CP 零分配库 cap:qpack（HTTP/3 QPACK RFC 9204），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:h2_encode` 吸收自 Bun http/h2_client/encode.cp 的纯 CP 零分配 HTTP/2 出站编码库：RFC 9113 请求头过滤、HPACK 动态表更新、客户端序言、HEADERS/CONTINUATION 分片与双窗口 DATA 帧编码（5/5 PASS，自动动态挂载 `cap.h2_encode`）
吸收 Bun 上游 src/http/h2_client/encode.cp 为纯 CP 零分配库 cap:h2_encode，含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:mds` 纯 CP 零分配多维尺度分析（MDS）降维嵌入库：Torgerson-Gower 双中心化 Classical MDS、SMACOF 迭代应力最小化与 Kruskal Stress-1 评估（5/5 PASS，自动动态挂载 `cap.mds`）
新增纯 CP 零分配多维尺度分析库 cap:mds（Classical MDS、SMACOF 迭代精修、Kruskal Stress-1），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:particle` 纯 CP 零分配序贯蒙特卡洛与粒子滤波库：log-sum-exp 权重更新、ESS 有效样本数、Systematic/Stratified/Multinomial 重采样与非线性多信标追踪（5/5 PASS，自动动态挂载 `cap.particle`）
新增纯 CP 零分配序贯蒙特卡洛/粒子滤波库 cap:particle（Systematic/Stratified/Multinomial 重采样与 ESS 自适应粒子滤波），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:spectral` 纯 CP 零分配谱图聚类与图拉普拉斯分析库：非归一化/对称归一化拉普拉斯、Fiedler 代数连通度与谱二分、NJW k-路谱嵌入、NCut 图割度量与高斯 RBF 亲和矩阵（5/5 PASS，自动动态挂载 `cap.spectral`）
新增纯 CP 零分配谱图聚类与拉普拉斯库 cap:spectral（Fiedler 谱二分、NJW 谱嵌入、NCut、RBF 亲和矩阵），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:cusum` 纯 CP 零分配在线变点与流式异常检测库：双边 Page CUSUM、无先验 Page-Hinkley 漂移检测、Roberts 时变控制限 EWMA 控制图与 Welford 流式 Z-Score（5/5 PASS，自动动态挂载 `cap.cusum`）
新增纯 CP 零分配在线变点与异常检测库 cap:cusum（CUSUM、Page-Hinkley、EWMA 控制图、流式 Z-Score），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:holt` 纯 CP 零分配时间序列指数平滑与预测库：单指数平滑 SES、Holt 阻尼线性趋势、Holt-Winters 加法/乘法三指数季节预测与 MAE/RMSE/MAPE/sMAPE 评估（5/5 PASS，自动动态挂载 `cap.holt`）
新增纯 CP 零分配时间序列指数平滑与预测库 cap:holt（SES / Holt / Holt-Winters 与预测误差评估），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:iir` 纯 CP 零分配经典 IIR 滤波器设计库：频率预畸变双线性变换 Butterworth 低/高通与 Chebyshev-I 低通 SOS 级联、TDF-II 滤波、零相位 sosFiltFilt 与幅频 dB 响应（5/5 PASS，自动动态挂载 `cap.iir`）
新增纯 CP 零分配经典 IIR 滤波器设计库 cap:iir（Butterworth/Chebyshev-I 双线性 SOS 设计与零相位 filtfilt），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:anneal` 纯 CP 零分配模拟退火全局优化库：四种降温曲线、Metropolis 接受准则、连续盒约束 SA 与 2-opt/swap 离散排列 TSP SA（5/5 PASS，自动动态挂载 `cap.anneal`）
新增纯 CP 零分配模拟退火全局优化库 cap:anneal（四种降温曲线、连续盒约束 SA、2-opt 排列 TSP SA），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:savitzky` 纯 CP 零分配 Savitzky-Golay 多项式平滑/求导与 LOESS 局部加权回归库：动态 Vandermonde 卷积核、保峰 1/2 阶数值微分与 Cleveland Tricube 鲁棒 LOESS 平滑（5/5 PASS，自动动态挂载 `cap.savitzky`）
新增纯 CP 零分配 Savitzky-Golay 与 LOESS 平滑库 cap:savitzky（动态卷积核、数值微分、鲁棒 LOESS），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:huber` 纯 CP 零分配鲁棒统计与 M-估计回归库：median/MAD/trimmedMean/Huber 位置估计、Huber/Tukey Biweight IRLS 多元鲁棒回归与 Theil-Sen 1D 中位数斜率回归（5/5 PASS，自动动态挂载 `cap.huber`）
新增纯 CP 零分配鲁棒统计与 M-估计回归库 cap:huber（MAD/Trimmed/Huber/Tukey IRLS 与 Theil-Sen），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:isotonic` 纯 CP 零分配保序回归（Isotonic Regression / PAVA）与概率校准库：O(n) 加权非降/非增 PAVA、重复 x 合并分段线性插值与 Brier / ECE 校准评估（5/5 PASS，自动动态挂载 `cap.isotonic`）
新增纯 CP 零分配保序回归与概率校准库 cap:isotonic（PAVA、分段线性插值、Brier Score 与 ECE），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:biquad` 纯 CP 零分配音频/DSP Biquad IIR 滤波器与 Goertzel 单频检测库：RBJ Cookbook 8 种二阶节、Transposed Direct Form II、级联 SOS、解析频响与 O(n) Goertzel 检测（5/5 PASS，自动动态挂载 `cap.biquad`）
新增纯 CP 零分配 Biquad IIR 滤波与 Goertzel 检测库 cap:biquad（RBJ 8 种二阶节、SOS 级联、解析频响、Goertzel），含文档与 7 项深度生产测试

## [2026-11-22] - 新建 `cap:lasso` 纯 CP 零分配稀疏正则化回归库：增量残差循环坐标下降 + 软阈值统一求解 Lasso（L1）、Ridge（L2）与 ElasticNet（L1+L2）及 R²/MSE 评估（5/5 PASS，自动动态挂载 `cap.lasso`）
新增纯 CP 零分配稀疏正则化回归库 cap:lasso（Lasso / Ridge / ElasticNet 坐标下降求解器），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:louvain` 纯 CP 零分配 Louvain 模块度社区发现与标签传播库：带权无向 CSR 图、Newman-Girvan 模块度计算、精确 ΔQ 贪心局部移动、高分辨率细分与零分配 LPA（5/5 PASS，自动动态挂载 `cap.louvain`）
新增纯 CP 零分配社区发现库 cap:louvain（Louvain 模块度优化、Newman-Girvan 模块度、标签传播 LPA），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:newton` 纯 CP 零分配非线性方程组求解与非线性最小二乘库：中心差分雅可比阻尼 Newton-Raphson、自适应 Levenberg-Marquardt 非线性最小二乘与 1D Newton/割线求根（5/5 PASS，自动动态挂载 `cap.newton`）
新增纯 CP 零分配非线性方程组与 LM 最小二乘库 cap:newton（阻尼 Newton-Raphson、Levenberg-Marquardt、1D Newton/割线法），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:ga` 纯 CP 零分配遗传算法与演化计算库：双缓冲种群、实数编码 GA（锦标赛/BLX-α/高斯变异/精英保留）与排列编码 GA（Order Crossover OX/反转变异，专用于 TSP 与调度）（5/5 PASS，自动动态挂载 `cap.ga`）
新增纯 CP 零分配遗传算法库 cap:ga（实数编码 GA 与排列编码 OX/TSP GA），含文档与 5 项深度生产测试

## [2026-11-22] - 新建 `cap:lsh` 纯 CP 零分配局部敏感哈希（LSH）近似最近邻检索库：随机超平面余弦 LSH、p-稳定分布欧氏 E2LSH、多表固定池桶索引与 O(1) 位图去重候选查询（5/5 PASS，自动动态挂载 `cap.lsh`）
新增纯 CP 零分配局部敏感哈希库 cap:lsh（随机超平面余弦 LSH 与 p-稳定分布欧氏 E2LSH），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:nmf` 纯 CP 零分配非负矩阵分解（NMF）库：Lee-Seung Frobenius 与广义 KL 散度乘法更新、确定性初始化、固定基矩阵新样本投影与重构误差评估（5/5 PASS，自动动态挂载 `cap.nmf`）
新增纯 CP 零分配非负矩阵分解库 cap:nmf（Lee-Seung Frobenius / KL 散度 NMF 与新样本投影），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:naivebayes` 纯 CP 零分配朴素贝叶斯（Naive Bayes）分类库：Gaussian（方差平滑）、Multinomial（Laplace/Lidstone 平滑）与 Bernoulli（阈值二值化）全对数域 log-sum-exp 防下溢分类器（5/5 PASS，自动动态挂载 `cap.naivebayes`）
新增纯 CP 零分配朴素贝叶斯分类库 cap:naivebayes（Gaussian / Multinomial / Bernoulli 全对数域分类器），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:rrt` 纯 CP 零分配 2D 采样运动规划库（RRT & RRT*）：固定节点池、圆/矩形障碍物精确碰撞检测、RRT* 邻域最优父节点选择与子树重连及视线路径平滑（5/5 PASS，自动动态挂载 `cap.rrt`）
新增纯 CP 零分配 2D 采样运动规划库 cap:rrt（RRT / RRT*、障碍物碰撞检测、视线路径平滑），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:logreg` 纯 CP 零分配二分类与多分类 Softmax 逻辑回归库：数值稳定 sigmoid/softmax、Armijo 回溯 L2 正则二分类与多分类求解及 TP/FP/TN/FN/F1 评估指标（5/5 PASS，自动动态挂载 `cap.logreg`）
新增纯 CP 零分配逻辑回归库 cap:logreg（二分类/多分类 Softmax L2 正则逻辑回归与分类指标），含文档与 7 项深度生产测试

## [2026-11-22] - 新建 `cap:dtree` 纯 CP 零分配全迭代 CART 决策树分类与回归库：固定节点池、Gini/Entropy/MSE 最优轴对齐划分、深度/叶样本剪枝与加权不纯度下降特征重要性（5/5 PASS，自动动态挂载 `cap.dtree`）
新增纯 CP 零分配 CART 决策树分类与回归库 cap:dtree（全迭代 Gini/Entropy/MSE、特征重要性、池耗尽优雅退化），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:pso` 纯 CP 零分配全局启发式优化库：线性衰减惯性权重粒子群优化（PSO）与经典 DE/rand/1/bin 差分进化，内置确定性 SplitMix64 与盒约束边界保护（5/5 PASS，自动动态挂载 `cap.pso`）
新增纯 CP 零分配全局启发式优化库 cap:pso（粒子群优化 PSO 与差分进化 DE），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:svm` 纯 CP 零分配支持向量机（SVM）分类库：Linear/Poly/RBF 核函数、Platt 启发式 SMO 对偶二次规划求解器与 Pegasos 高速线性 SVM（5/5 PASS，自动动态挂载 `cap.svm`）
新增纯 CP 零分配支持向量机库 cap:svm（Platt SMO 核 SVM、Linear/Poly/RBF 核、Pegasos 线性 SVM），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:gmm` 纯 CP 零分配高斯混合模型（GMM / EM）聚类与密度估计库：对角协方差多维高斯混合、k-means++ 初始化、log-sum-exp 稳定 EM、AIC/BIC 与后验责任度预测（5/5 PASS，自动动态挂载 `cap.gmm`）
新增纯 CP 零分配高斯混合模型库 cap:gmm（对角协方差 GMM / EM、AIC/BIC、对数密度与后验预测），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:dbscan` 纯 CP 零分配基于密度的空间聚类库（DBSCAN）：Ester 1996 核心/边界/噪声点语义、四种距离度量、预计算距离矩阵聚类与轮廓系数评估（5/5 PASS，自动动态挂载 `cap.dbscan`）
新增纯 CP 零分配密度聚类库 cap:dbscan（DBSCAN 聚类、距离矩阵聚类、轮廓系数），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:bandit` 纯 CP 零分配多臂老虎机在线决策库：UCB1、Epsilon-Greedy（含衰减）、Marsaglia-Tsang Beta-Bernoulli Thompson Sampling、对数权重防溢出 EXP3 与累积遗憾计算（5/5 PASS，自动动态挂载 `cap.bandit`）
新增纯 CP 零分配多臂老虎机在线决策库 cap:bandit（UCB1、Epsilon-Greedy、Thompson Sampling、EXP3），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:dtw` 纯 CP 零分配动态时间规整（DTW）与时间序列对齐库：1D 滚动双行 O(m) DTW、Sakoe-Chiba 带宽约束、最优路径回溯、多维 DTW、LB_Keogh 下界包络与 Z-归一化（5/5 PASS，自动动态挂载 `cap.dtw`）
新增纯 CP 零分配动态时间规整库 cap:dtw（1D/多维 DTW、Sakoe-Chiba 带宽、路径回溯、LB_Keogh 下界、Z-归一化），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:mcts` 纯 CP 零分配蒙特卡洛树搜索（MCTS / UCT & PUCT）决策规划库：左孩子右兄弟固定节点池、UCB1/PUCT 下探选择、先验展开、单人/双人零和回传与根节点策略统计（5/5 PASS，自动动态挂载 `cap.mcts`）
新增纯 CP 零分配蒙特卡洛树搜索库 cap:mcts（固定池 UCT / PUCT、零和回传、井字棋残局决策规划），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:bessel` 纯 CP 零分配柱贝塞尔与修正贝塞尔特殊函数库：Double-Double 106-bit 无损幂级数 + Hankel 渐近 + Miller 向下递推 + 指数缩放防溢出/下溢（5/5 PASS，自动动态挂载 `cap.bessel`）
新增纯 CP 零分配贝塞尔特殊函数库 cap:bessel（j0/j1/jn、y0/y1/yn、besselI0/besselI1/i0e/i1e、k0/k1/k0e/k1e），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:ekf` 纯 CP 零分配非线性状态估计库（EKF & UKF）：中心差分雅可比、Joseph 形式对称正定扩展卡尔曼滤波、新息 NIS 计算与 Merwe Sigma 点无迹卡尔曼滤波预测（5/5 PASS，自动动态挂载 `cap.ekf`）
新增纯 CP 零分配非线性状态估计库 cap:ekf（EKF Joseph 更新、数值雅可比、UKF Sigma 点预测），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:markov` 纯 CP 零分配离散时间马尔可夫链（DTMC）分析与采样库：随机矩阵校验、快速幂、平稳分布幂迭代、吸收链基本矩阵 N=(I-Q)^-1、熵率与确定性轨迹采样（5/5 PASS，自动动态挂载 `cap.markov`）
新增纯 CP 零分配离散时间马尔可夫链库 cap:markov（快速幂、平稳分布、吸收链基本矩阵、熵率、确定性轨迹采样），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:bfgs` 纯 CP 零分配拟牛顿法与梯度优化库：完整逆 Hessian BFGS、双循环有限内存 L-BFGS（O(mn) 内存）、Armijo/Wolfe 线搜索与中心差分梯度（5/5 PASS，自动动态挂载 `cap.bfgs`）
新增纯 CP 零分配拟牛顿优化库 cap:bfgs（BFGS、L-BFGS、Armijo/Wolfe 线搜索、中心差分梯度），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:gamma` 纯 CP 零分配特殊数学函数库：Lanczos lgamma/tgamma、digamma、级数/Lentz 连分式正则化不完全 Gamma/Beta、无灾难抵消 erf/erfc 与对数阶乘/组合数（5/5 PASS，自动动态挂载 `cap.gamma`）
新增纯 CP 零分配特殊数学函数库 cap:gamma（lgamma/tgamma/digamma/gammaP/gammaQ/betainc/erf/erfc），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:ransac` 纯 CP 零分配确定性 RANSAC 鲁棒模型估计库：SplitMix64 采样与内点精修，支持 2D 直线（TLS）、2D 圆（Kåsa）、3D 平面（协方差最小特征向量）与 1D 线性回归（OLS）（5/5 PASS，自动动态挂载 `cap.ransac`）
新增纯 CP 零分配 RANSAC 鲁棒模型估计库 cap:ransac（2D 直线/圆、3D 平面、1D 线性回归 + 内点精修），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:nelder` 纯 CP 零分配无导数优化与一维求极值/求根库：Nelder-Mead 单纯形优化、一维黄金分割极小化与 Brent 超线性求根法（5/5 PASS，自动动态挂载 `cap.nelder`）
新增纯 CP 零分配无导数优化与一维求根库 cap:nelder（Nelder-Mead、黄金分割、Brent 求根），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:quadrature` 纯 CP 零分配高精度数值积分库：Gauss-Legendre 5/10 点、复合 Simpson 1/3、无递归显式栈自适应 Gauss-Kronrod G7-K15 与 Romberg Richardson 外推积分（5/5 PASS，自动动态挂载 `cap.quadrature`）
新增纯 CP 零分配高精度数值积分库 cap:quadrature（Gauss-Legendre 5/10、复合 Simpson、自适应 G7-K15、Romberg 外推），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:kmp` 纯 CP 零分配线性时间字符串匹配与回文算法库：KMP 前缀表/重叠匹配/最小周期、Z-Algorithm 与无哨兵 Manacher 最长回文子串（支持任意二进制字节串）（5/5 PASS，自动动态挂载 `cap.kmp`）
新增纯 CP 零分配线性时间字符串与回文算法库 cap:kmp（KMP、Z-Algorithm、无哨兵 Manacher），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:hmm` 纯 CP 零分配离散隐马尔可夫模型（HMM）库：对数域 Viterbi 最优路径解码、Rabiner 缩放 Forward-Backward 对数似然与后验分布及 Baum-Welch EM 参数重估（5/5 PASS，自动动态挂载 `cap.hmm`）
新增纯 CP 零分配隐马尔可夫模型库 cap:hmm（对数 Viterbi、缩放 Forward-Backward、Baum-Welch EM），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:suffix` 纯 CP 零分配后缀数组（Suffix Array）与 Kasai LCP 字符串索引库：倍增基数排序 O(n log n) SA、Kasai O(n) LCP、二分模式区间定位、不同子串计数与最长重复子串（5/5 PASS，自动动态挂载 `cap.suffix`）
新增纯 CP 零分配后缀数组与 Kasai LCP 索引库 cap:suffix（SA、LCP、模式区间定位、不同子串计数、最长重复子串），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:voronoi` 纯 CP 零分配 2D 有界 Voronoi 图与 Lloyd 松弛几何库：Sutherland-Hodgman 半平面裁剪、双缓冲 Voronoi 单元构建、面积/质心度量与 Lloyd 松弛迭代（5/5 PASS，自动动态挂载 `cap.voronoi`）
新增纯 CP 零分配 2D 有界 Voronoi 图与 Lloyd 松弛库 cap:voronoi（半平面裁剪、Voronoi 单元、Lloyd 松弛、最近站点），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:bipartite` 纯 CP 零分配二分图算法库：BFS 二染色二分性判定、Hopcroft-Karp O(E√V) 最大基数匹配与 Kőnig 定理最小点覆盖/最大独立集提取（5/5 PASS，自动动态挂载 `cap.bipartite`）
新增纯 CP 零分配二分图与最大匹配库 cap:bipartite（BFS 二染色、Hopcroft-Karp 最大匹配、Kőnig 最小点覆盖），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:delaunay` 纯 CP 零分配 2D Delaunay 三角剖分库：Bowyer-Watson 增量剖分、无除法外接圆行列式谓词、CCW 绕序保证、空外接圆准则检验与三角形点定位（5/5 PASS，自动动态挂载 `cap.delaunay`）
新增纯 CP 零分配 2D Delaunay 三角剖分库 cap:delaunay（Bowyer-Watson 剖分、几何谓词、空外接圆检验、点定位），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:treap` 纯 CP 零分配随机平衡树与顺序统计 Treap 库：固定节点池与空闲链表复用、SplitMix64 优先级、O(log n) kth/rank/lowerBound/upperBound/rangeCount 与不变量校验（5/5 PASS，自动动态挂载 `cap.treap`）
新增纯 CP 零分配随机平衡树/顺序统计库 cap:treap（固定池 Treap、空闲链表复用、O(log n) 顺序统计与范围计数），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:knn` 纯 CP 零分配 K-最近邻（KNN）分类与回归库：欧氏/曼哈顿/切比雪夫/余弦四种距离、原地大顶堆 Top-K 搜索、均匀/反距离加权分类与回归及留一法交叉验证（5/5 PASS，自动动态挂载 `cap.knn`）
新增纯 CP 零分配 KNN 分类与回归库 cap:knn（四种距离、Top-K 堆搜索、分类/回归、留一法 LOO-CV），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:pca` 纯 CP 零分配主成分分析（PCA）与实对称矩阵 Jacobi 特征分解库：循环 Jacobi 特征分解、无偏协方差 PCA 拟合、降维投影、逆变换重构与均方误差检验（5/5 PASS，自动动态挂载 `cap.pca`）
新增纯 CP 零分配主成分分析与对称特征分解库 cap:pca（Jacobi 对称特征分解、PCA 拟合/投影/重构），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:svd` 纯 CP 零分配单边 Jacobi 奇异值分解与广义逆库：经济型 SVD（含零奇异值正交补全）、截断奇异值最小范数最小二乘与 Moore-Penrose 伪逆（5/5 PASS，自动动态挂载 `cap.svd`）
新增纯 CP 零分配单边 Jacobi SVD 与广义逆库 cap:svd（Jacobi SVD、最小范数最小二乘、Moore-Penrose 伪逆），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:qr` 纯 CP 零分配 Householder QR 分解与线性最小二乘库：原地 Householder 反射分解、Q^T 乘法、经济型 Q/R 构造、超定最小二乘求解与极端量级数值稳定防护（5/5 PASS，自动动态挂载 `cap.qr`）
新增纯 CP 零分配 Householder QR 分解与最小二乘库 cap:qr（原地 QR、Q^T 应用、经济型 Q/R、超定最小二乘），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:lu` 纯 CP 零分配部分选主元 LU 分解与三对角求解库：Doolittle PA=LU 分解、多右端项求解、行列式、矩阵求逆、残差检验与 O(n) Thomas 三对角追赶法（5/5 PASS，自动动态挂载 `cap.lu`）
新增纯 CP 零分配 LU 分解与三对角求解库 cap:lu（PA=LU、线性求解、行列式、矩阵求逆、Thomas 三对角法），含文档与 6 项深度生产测试

## [2026-11-22] - 新建 `cap:simplex` 纯 CP 零分配两阶段线性规划单纯形求解器库：Bland 防退化循环规则、混合 <=/>=/= 约束、Phase-I 人工变量、无界/不可行检出与原/对偶影子价格提取（5/5 PASS，自动动态挂载 `cap.simplex`）
新增纯 CP 零分配两阶段单纯形线性规划库 cap:simplex（Bland 规则、混合约束、无界/不可行检出与对偶影子价格），含文档与 7 项深度生产测试

## [2026-11-22] - 新建 `cap:mst` 纯 CP 零分配最小生成树与生成森林算法库：路径压缩并查集、Kruskal 最小/最大生成树、稠密图 O(V²) Prim 生成森林与生成树无环连通校验（5/5 PASS，自动动态挂载 `cap.mst`）
新增纯 CP 零分配最小生成树与生成森林库 cap:mst（并查集、Kruskal、Prim、最大生成树、生成树校验），含文档与 6 项深度生产测试

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

