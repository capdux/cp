# `cap:dtree` — 纯 CP 零堆分配 CART 决策树分类与回归库

`cap:dtree` 提供 100% 纯 CP、零堆分配、全迭代无递归的 CART（Classification and Regression Trees）决策树库，支持多分类（Gini 不纯度 / 香农 Entropy 信息增益）与连续值回归（MSE 方差最小化），并内置基于加权不纯度下降的归一化特征重要性计算。

## 特性概览

- **零堆分配（Zero-Alloc）**：固定节点池 `nodes_buf` 与训练工作区 `TreeWorkspace`（索引、排序缓冲、类别计数与迭代 DFS 栈）全部由调用方提供。
- **全迭代无递归**：采用显式工作栈建树 + 自底向上迭代稳定归并排序（$O(m \log m)$ 最坏复杂度），无栈溢出风险。
- **分类与回归统一架构**：
  - 分类：`SplitCriterion.Gini`（$O(1)$ 增量更新）与 `SplitCriterion.Entropy`（以 2 为底香农熵）。
  - 回归：`SplitCriterion.Mse`（中心化平移增量方差下降，高精度防相消）。
- **优雅池耗尽退化**：当固定节点池或迭代栈在训练中途耗尽时，活跃节点自动优雅退化为叶节点，返回合法可预测的决策树。
- **完备边界防御**：自动拦截空数据、越界类别标签、缓冲不足、准则不匹配及 `NaN`/`Inf` 输入。

## API 参考

### 常量与枚举
- `DTREE_NO_NODE: u32 = 0xFFFFFFFF`：子节点空哨兵索引。
- `SplitCriterion`：`Gini` | `Entropy` | `Mse`。

### 核心数据结构
- `TreeNode`：`is_leaf`, `feature_idx`, `threshold`, `left`, `right`, `predicted_class`, `predicted_value`, `sample_count`, `impurity`；提供 `TreeNode.empty()`。
- `TreeConfig`：`max_depth`, `min_samples_split`, `min_samples_leaf`, `criterion`；提供 `TreeConfig.defaultClassifier()` / `TreeConfig.defaultRegressor()`。
- `TreeWorkspace`：`sample_indices: []usize`、`sort_tmp: []usize`、`class_counts: []usize`、`left_counts: []usize`、`stack_node: []u32`、`stack_lo: []usize`、`stack_hi: []usize`、`stack_depth: []usize`。

### `DecisionTree` 方法
- `DecisionTree.fitClassifier(x: []const f64, y: []const i32, n_samples: usize, n_features: usize, num_classes: usize, cfg: TreeConfig, nodes_buf: *[]TreeNode, ws: *TreeWorkspace): DecisionTree?`
- `DecisionTree.fitRegressor(x: []const f64, y: []const f64, n_samples: usize, n_features: usize, cfg: TreeConfig, nodes_buf: *[]TreeNode, ws: *TreeWorkspace): DecisionTree?`
- `predictClass(sample: []const f64): i32?`
- `predictValue(sample: []const f64): f64?`
- `nodeCount(): usize` / `leafCount(): usize` / `depth(): usize`
- `featureImportances(out: *[]f64): bool`

## 使用示例

```cp
import {
    SplitCriterion,
    TreeNode,
    TreeConfig,
    TreeWorkspace,
    DecisionTree,
} from "cap:dtree";

const x: [12]f64 = [
    -2.0, -1.0,  -1.8, -0.5,
     0.0,  2.0,   0.2,  2.4,
     2.5, -1.5,   2.8, -1.0,
];
const y: [6]i32 = [0, 0, 1, 1, 2, 2];

let pool_b: [32]TreeNode = [TreeNode.empty(); 32];
let nodes_buf: []TreeNode = pool_b[0..];

let idx_b: [6]usize = [0; 6];
let tmp_b: [6]usize = [0; 6];
let cc_b: [3]usize = [0; 3];
let lc_b: [3]usize = [0; 3];
let sn_b: [32]u32 = [0; 32];
let slo_b: [32]usize = [0; 32];
let shi_b: [32]usize = [0; 32];
let sd_b: [32]usize = [0; 32];

let ws = TreeWorkspace {
    sample_indices: idx_b[0..],
    sort_tmp: tmp_b[0..],
    class_counts: cc_b[0..],
    left_counts: lc_b[0..],
    stack_node: sn_b[0..],
    stack_lo: slo_b[0..],
    stack_hi: shi_b[0..],
    stack_depth: sd_b[0..],
};

const cfg = TreeConfig.defaultClassifier();
if (tree := DecisionTree.fitClassifier(x[0..], y[0..], 6, 2, 3, cfg, *nodes_buf, *ws)) {
    const q: [2]f64 = [0.1, 2.2];
    const pred = tree.predictClass(q[0..]); // Some(1)
    * = pred;
}
```
