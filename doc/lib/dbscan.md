# `cap:dbscan` — 零分配基于密度的空间聚类库（DBSCAN）

零堆分配（Zero-Alloc）的纯 CP DBSCAN（Density-Based Spatial Clustering of Applications with Noise，Ester et al. 1996）聚类算法库，支持任意非凸形状簇发现、核心点/边界点/噪声点精确区分、4 种距离度量、预计算距离矩阵聚类，以及零分配聚类轮廓系数（Silhouette Score）评估。

## 核心特性

- **零堆分配（Zero-Alloc）**：BFS 扩展队列、入队防重表与邻域临时切片全部通过 `DbscanWorkspace` 由调用方预分配。
- **严格 Ester (1996) 语义**：准确识别核心点（`is_core == 1`）、密度可达边界点（`is_core == 0, label >= 0`）及噪声离群点（`label == DBSCAN_NOISE`），按确定性扫描顺序分配连续簇 ID `0..num_clusters-1`。
- **4 种距离度量**：支持 `Euclidean`（L2）、`Manhattan`（L1）、`Chebyshev`（L∞）与 `Cosine`（余弦距离 `1 - cos`）。
- **自定义距离矩阵**：`dbscanFromDistanceMatrix` 直接在调用方预计算的任意 `n x n` 非负距离矩阵上聚类。
- **无堆轮廓系数**：`silhouetteScore` 忽略噪声点评估非噪声簇分离度（`[-1, 1]`）。

## 导出 API

### 常量与数据模型

- `DBSCAN_NOISE: i32 = -1`、`DBSCAN_UNVISITED: i32 = -2`
- `DbscanMetric { Euclidean, Manhattan, Chebyshev, Cosine }`
- `DbscanResult { num_clusters: usize, noise_count: usize, core_count: usize }`
- `DbscanWorkspace { queue: []usize, in_queue: []u8, neighbors_tmp: []usize }`

### 核心函数

- `dbscanCluster(points: []const f64, n_samples: usize, n_features: usize, eps: f64, min_pts: usize, metric: DbscanMetric, labels_out: []i32, is_core_out: []u8, ws: *DbscanWorkspace): DbscanResult?`
- `dbscanFromDistanceMatrix(dist_mat: []const f64, n_samples: usize, eps: f64, min_pts: usize, labels_out: []i32, is_core_out: []u8, ws: *DbscanWorkspace): DbscanResult?`
- `silhouetteScore(points: []const f64, n_samples: usize, n_features: usize, labels: []const i32, metric: DbscanMetric): f64?`

## 使用示例

```cp
import std from "std";
import {
    DbscanMetric,
    DbscanWorkspace,
    dbscanCluster,
    silhouetteScore,
} from "cap:dbscan";

fn example() {
    const pts: [12]f64 = [
        0.0, 0.0,  0.1, 0.0,  0.0, 0.1,
        5.0, 5.0,  5.1, 5.0,  5.0, 5.1,
    ];
    let labels: [6]i32 = std.mem.zeroes([6]i32);
    let core: [6]u8 = std.mem.zeroes([6]u8);
    let q: [6]usize = std.mem.zeroes([6]usize);
    let inq: [6]u8 = std.mem.zeroes([6]u8);
    let nb: [6]usize = std.mem.zeroes([6]usize);
    let ws = DbscanWorkspace { queue: q[0..], in_queue: inq[0..], neighbors_tmp: nb[0..] };

    if (res := dbscanCluster(pts[0..], 6, 2, 0.5, 2, DbscanMetric.Euclidean, labels[0..], core[0..], &ws)) {
        * = res.num_clusters; // 2
        * = silhouetteScore(pts[0..], 6, 2, labels[0..], DbscanMetric.Euclidean);
    }
}
```
