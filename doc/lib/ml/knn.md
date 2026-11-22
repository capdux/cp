# `cap:knn` — 零堆分配 K-最近邻（KNN）分类与回归库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ml`，导入请使用 `import { ... } from "cap:ml"`。

`cap:knn` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的 K-最近邻检索、分类、回归及留一法交叉验证（LOO-CV）算法库。所有 Top-K 堆维护与加权投票均在调用方提供的固定缓冲中原地完成。

## 核心特性

- **100% 零堆分配**：基于原地大顶堆维护 Top-K（$O(N \log K)$），无需任何动态内存或哈希表分配。
- **四种距离度量**：欧氏距离（`Euclidean`）、曼哈顿距离（`Manhattan`）、切比雪夫距离（`Chebyshev`）与余弦距离（`Cosine`）。
- **确定性平局处理**：距离相等时严格按训练样本索引升序打破平局。
- **双加权模式**：支持均匀投票/均值（`Uniform`）与反距离加权（`InverseDistance`），并对零距离精确命中做特殊直通/多点平均处理。
- **内置模型评估**：提供零分配留一法交叉验证 `knnLeaveOneOutAccuracy`。

---

## 导出清单

### 枚举与结构

- `DistanceMetric`：`Euclidean`, `Manhattan`, `Chebyshev`, `Cosine`
- `WeightMode`：`Uniform`, `InverseDistance`
- `Neighbor`：`{ index: usize, distance: f64 }`

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `distanceMetric(a: []const f64, b: []const f64, metric: DistanceMetric): f64` | 计算两个等长特征向量距离；非法输入返回 `-1.0` |
| `findKNearest(train_x: []const f64, n_samples: usize, n_features: usize, query: []const f64, k: usize, metric: DistanceMetric, out_neighbors: *[]Neighbor): usize?` | 查找距离升序的 Top-K 近邻，返回 `k` |
| `knnClassify(train_x, train_labels: []const i32, n_samples, n_features, query, k, metric, weight_mode, neighbors_scratch: *[]Neighbor): i32?` | KNN 离散分类预测 |
| `knnRegress(train_x, train_y: []const f64, n_samples, n_features, query, k, metric, weight_mode, neighbors_scratch: *[]Neighbor): f64?` | KNN 连续值回归预测 |
| `knnLeaveOneOutAccuracy(train_x, train_labels: []const i32, n_samples, n_features, k, metric, weight_mode, neighbors_scratch: *[]Neighbor): f64?` | 留一法交叉验证分类准确率（`[0.0, 1.0]`） |

---

## 使用示例

```typescript
import std from "std";
import {
    DistanceMetric,
    WeightMode,
    Neighbor,
    knnClassify,
    knnRegress,
    knnLeaveOneOutAccuracy,
} from "cap:ml";

fn knnDemo() {
    const train_x = [
        0.0, 0.0,
        0.2, 0.1,
        10.0, 10.0,
        10.2, 9.8,
    ];
    const labels: [4]i32 = [0, 0, 1, 1];
    const targets = [1.0, 1.2, 20.0, 19.8];
    const query = [0.1, 0.05];

    let scratch: [3]Neighbor = std.mem.zeroes([3]Neighbor);
    let s: []Neighbor = scratch[0..];

    const cls = knnClassify(train_x[0..], labels[0..], 4, 2, query[0..], 3, .Euclidean, .InverseDistance, &s);
    const val = knnRegress(train_x[0..], targets[0..], 4, 2, query[0..], 2, .Euclidean, .Uniform, &s);
    const acc = knnLeaveOneOutAccuracy(train_x[0..], labels[0..], 4, 2, 1, .Euclidean, .Uniform, &s);
    * = cls;
    * = val;
    * = acc;
}
```
