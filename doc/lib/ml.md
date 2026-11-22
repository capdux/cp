# CP 标准库：经典机器学习、聚类与决策搜索套件手册 (`cap:ml`)

> 来源：Pure-CP 经典机器学习、概率统计、矩阵优化、图排序与 AI 护栏标准库收敛（26 个算法子模块、27 个实现文件统一聚合） | 更新日期：2026-11-22

## 一、 架构总览

`cap:ml` 是 CP 语言官方内置的纯原生、零堆分配（Zero-Alloc）经典机器学习、聚类、降维、稳健回归、近似检索与序贯决策标准库。所有算法均以显式工作区或切片缓冲运行，无外部 C 依赖，专为边缘 AI Agent 推理、在线特征分析与实时决策打造。

---

## 二、 模块文件拓扑与核心 API 索引

### 1. 分类、回归与降维（11 个子模块）

| 子模块文件 | 算法领域 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/ml/svm.cp` | Platt SMO 核支持向量机 & Pegasos 线性 SVM | `svmFitSmo`, `svmDecision`, `svmPredict`, `linearSvmFit`, `linearSvmDecision`, `evalKernel`, `KernelKind`, `SmoConfig`, `SmoResult` |
| `lib/ml/knn.cp` | K-近邻分类/回归与留一交叉验证 | `findKNearest`, `knnClassify`, `knnRegress`, `knnLeaveOneOutAccuracy`, `distanceMetric`, `DistanceMetric`, `WeightMode`, `Neighbor` |
| `lib/ml/dtree.cp` | CART 决策树分类器与回归器 | `DecisionTree`, `TreeNode`, `TreeConfig`, `TreeWorkspace`, `SplitCriterion`, `DTREE_NO_NODE` |
| `lib/ml/logreg.cp` | 二分类与多分类 L2 逻辑回归 | `fitBinary`, `predictProbaBinary`, `predictBinary`, `fitMultinomial`, `predictProbaMultinomial`, `predictClassMultinomial`, `sigmoid`, `softmax`, `logLossBinary`, `binaryMetrics` |
| `lib/ml/naivebayes.cp` | 高斯 / 多项式 / 伯努利朴素贝叶斯 | `fitGaussianNb`, `gaussianNbPredict`, `fitMultinomialNb`, `multinomialNbPredict`, `fitBernoulliNb`, `bernoulliNbPredict` |
| `lib/ml/lasso.cp` | Lasso / Ridge / ElasticNet 稀疏正则回归 | `lassoFit`, `ridgeFit`, `elasticNetFit`, `linearPredict`, `softThreshold`, `mseScore`, `r2Score`, `LassoConfig`, `LassoWorkspace` |
| `lib/ml/isotonic.cp` | PAVA 保序回归与概率校准 | `fitIsotonic`, `isotonicPredict`, `pavaIncreasing`, `pavaDecreasing`, `brierScore`, `expectedCalibrationError`, `IsotonicModel` |
| `lib/ml/huber.cp` | 稳健统计、Huber/Tukey IRLS 与 Theil-Sen | `huberRegress`, `theilSen1D`, `huberLocation`, `huberLoss`, `huberWeight`, `tukeyBiweight`, `median`, `madScale`, `trimmedMean` |
| `lib/ml/nmf.cp` | 非负矩阵分解 (Lee-Seung 乘法更新) | `nmfFit`, `nmfTransform`, `nmfReconstruct`, `nmfFrobeniusError`, `nmfKlDivergence`, `NmfLoss`, `NmfConfig`, `NmfWorkspace` |
| `lib/ml/mds.cp` | 经典多维标度 (MDS) 与 SMACOF 精化 | `classicalMds`, `smacofRefine`, `pairwiseEuclidean`, `kruskalStress1`, `doubleCenterGram`, `MdsResult`, `SmacofResult` |
| `lib/ml/ransac.cp` | RANSAC 稳健几何拟合 (2D 线/圆、3D 平面、1D 线性) | `ransacFitLine2D`, `ransacFitCircle2D`, `ransacFitPlane3D`, `ransacFitLinear1D`, `RansacConfig`, `Point2`, `Point3` |

### 2. 聚类、社区发现与近似检索（5 个子模块）

| 子模块文件 | 算法领域 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/ml/dbscan.cp` | DBSCAN 密度聚类与轮廓系数 | `dbscanCluster`, `dbscanFromDistanceMatrix`, `silhouetteScore`, `DbscanMetric`, `DbscanResult`, `DbscanWorkspace`, `DBSCAN_NOISE` |
| `lib/ml/gmm.cp` | 高斯混合模型 (EM 拟合与对数似然) | `gmmFit`, `gmmPredict`, `gmmPredictProba`, `gmmLogProbSample`, `gmmScoreSamples`, `GmmConfig`, `GmmFitResult` |
| `lib/ml/spectral.cp` | 谱图聚类、拉普拉斯矩阵与 NJW 嵌入 | `buildLaplacian`, `fiedlerBisection`, `spectralEmbedding`, `normalizedCut`, `rbfAffinityMatrix`, `LaplacianKind`, `FiedlerResult` |
| `lib/ml/louvain.cp` | Louvain 模块度社区发现与标签传播 | `louvainDetect`, `labelPropagation`, `computeModularity`, `WeightedGraph`, `LouvainConfig`, `LouvainResult`, `LouvainWorkspace` |
| `lib/ml/lsh.cp` | 局部敏感哈希 (余弦 / 欧氏近似最近邻) | `CosineLsh`, `EuclideanLsh`, `estimatedCosineFromHamming`, `LshEntry`, `LSH_NO_ENTRY` |

### 3. 序贯决策与树搜索（2 个子模块）

| 子模块文件 | 算法领域 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/ml/bandit.cp` | 多臂老虎机 (UCB1 / ε-Greedy / Thompson / EXP3) | `Ucb1Bandit`, `EpsilonGreedyBandit`, `ThompsonBandit`, `Exp3Bandit`, `cumulativeRegret` |
| `lib/ml/mcts.cp` | 蒙特卡洛树搜索 (MCTS / UCT) | `MctsTree`, `MctsNode`, `MctsConfig`, `MCTS_NO_NODE` |

### 4. K-Means++、图排序、概率统计、矩阵优化与 AI 数据去重/安全护栏（8 个子模块）

| 子模块文件 | 算法领域 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/ml/kmeans.cp` | D 维 K-Means++ 聚类与 Lloyd 迭代 | `kmeansFit`, `kmeansInitPlusPlus`, `kmeansPredictOne`, `kmeansInertia`, `kmeansSilhouetteScore`, `sqDist`, `KmeansResult` |
| `lib/ml/pagerank.cp` | CSR 稀疏有向图 PageRank / PPR / HITS | `CsrGraph`, `pagerank`, `personalizedPagerank`, `hits`, `topNode`, `sumScores`, `PageRankResult`, `HitsResult` |
| `lib/ml/polyfit.cp` | 多项式最小二乘拟合、Givens QR 与 Ridge 回归 | `polyfit`, `polyfitRidge`, `linearFit`, `polyEval`, `polyDerivEval`, `polyRootBisect`, `polyRootNewton`, `evalFitMetrics` |
| `lib/ml/prob.cp` | 概率分布、确定性 RNG 与 A/B 假设检验 | `Rng`, `NormalDist`, `ExponentialDist`, `BinomialDist`, `PoissonDist`, `HypothesisTest`, `AbTestResult`, `ProbError` |
| `lib/ml/optimize.cp` | 1D 求根/极小化与多维非线性优化 | `Optimizer`, `OptError`, `ScalarFn`, `VectorFn`, `GradFn`, `RootResult`, `MinResult1D`, `OptResult` |
| `lib/ml/linalg.cp` | 任意维稠密矩阵运算与线性最小二乘 | `Matrix`, `LinalgError`, `dot`, `vecNorm1`, `vecNorm2`, `vecNormInf`, `leastSquares` |
| `lib/ml/dedup.cp` | AI 语料近重复文档指纹去重（SimHash / MinHash / LSH） | `SimHash`, `SimHashIndex`, `MinHash`, `LshIndex`, `SimHashHit`, `LshHit`, `DedupError` |
| `lib/ml/pii.cp` / `lib/ml/policy.cp` | AI Agent 安全护栏、PII 扫描脱敏与多级策略拦截 | `PiiScanner`, `PiiKind`, `PiiFinding`, `RedactionResult`, `GuardrailPolicy`, `GuardrailAction`, `GuardrailVerdict`, `GuardrailError` |

---

## 三、 使用示例

```typescript
import {
    Ucb1Bandit,
    sigmoid,
    huberLocation,
    DbscanMetric,
} from "cap:ml";

pub fn demoMl() {
    // 1. 逻辑回归激活与稳健位置估计
    const p = sigmoid(1.5);
    const samples = [1.0, 1.1, 1.2, 100.0];
    const loc = huberLocation(&samples, 1.345, 20);
    * = p;
    * = loc;

    // 2. UCB1 多臂老虎机在线决策
    let counts: [3]u32 = [0, 0, 0];
    let values: [3]f64 = [0.0, 0.0, 0.0];
    let bandit = Ucb1Bandit.init(&counts, &values, 1.414);
    const arm = bandit.selectArm();
    bandit.update(arm, 1.0);
    * = DbscanMetric.Euclidean;
}
```

---

## 收敛子模块深度文档

- [bandit.md](ml/bandit.md)
- [dbscan.md](ml/dbscan.md)
- [dedup.md](ml/dedup.md)
- [dtree.md](ml/dtree.md)
- [gmm.md](ml/gmm.md)
- [guardrail.md](ml/guardrail.md)
- [huber.md](ml/huber.md)
- [isotonic.md](ml/isotonic.md)
- [kmeans.md](ml/kmeans.md)
- [knn.md](ml/knn.md)
- [lasso.md](ml/lasso.md)
- [linalg.md](ml/linalg.md)
- [logreg.md](ml/logreg.md)
- [louvain.md](ml/louvain.md)
- [lsh.md](ml/lsh.md)
- [mcts.md](ml/mcts.md)
- [mds.md](ml/mds.md)
- [naivebayes.md](ml/naivebayes.md)
- [nmf.md](ml/nmf.md)
- [optimize.md](ml/optimize.md)
- [pagerank.md](ml/pagerank.md)
- [polyfit.md](ml/polyfit.md)
- [prob.md](ml/prob.md)
- [ransac.md](ml/ransac.md)
- [spectral.md](ml/spectral.md)
- [svm.md](ml/svm.md)
