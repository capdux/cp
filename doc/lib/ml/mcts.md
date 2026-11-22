# `cap:mcts` — 零分配蒙特卡洛树搜索（MCTS / UCT & PUCT）决策规划库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ml`，导入请使用 `import { ... } from "cap:ml"`。

零堆分配（Zero-Alloc）的纯 CP 蒙特卡洛树搜索（Monte Carlo Tree Search）核心框架，基于固定容量 `MctsNode` 节点池与左孩子右兄弟（First-Child / Next-Sibling）链表支持任意分支因子搜索树，原生提供经典 UCT（UCB1）与 AlphaZero 风格 PUCT 选择策略，自动兼容单人决策规划与双人零和交替博弈。

## 核心特性

- **零堆分配（Zero-Alloc）**：全部树节点存储在调用方预分配的 `[]MctsNode` 切片中；节点池满时 `expandNode` 自动安全截断，永不越界。
- **双策略选择（UCT & PUCT）**：支持经典 UCB1（未访问节点返回 `+inf` 保证探索）与带先验概率引导的 PUCT 公式 $Q + c_{\text{puct}} P \frac{\sqrt{N_{\text{parent}}}}{1 + N_{\text{child}}}$。
- **单人规划 & 双人零和博弈统一回传**：`backpropagate` 依据每个节点的父节点行动方 `parent.player` 是否等于 `root_player` 自动翻转收益符号，无缝支持交替对抗博弈与单智能体规划。
- **策略导出接口**：`bestActionByVisits`、`bestActionByMeanValue` 与 `rootActionStats` 直接输出根节点动作访问分布与平均收益，便于策略改进与自博弈训练。

## 导出 API

### 常量与数据模型

- `MCTS_NO_NODE: u32 = 0xFFFFFFFF`
- `MctsNode { parent, first_child, next_sibling, action, player, visits, total_value, prior, is_expanded, is_terminal }`
- `MctsConfig { exploration_c, use_puct, c_puct }`（`MctsConfig.defaultUct()` / `MctsConfig.defaultPuct()`）

### `MctsTree` 方法

- `MctsTree.init(nodes: []MctsNode, root_player: i8, cfg: MctsConfig): MctsTree?`
- `nodeCount(): usize`、`capacity(): usize`、`rootVisits(): u32`
- `ucb1Score(child_idx: u32, parent_visits: u32): f64`、`puctScore(child_idx: u32, parent_visits: u32): f64`
- `selectLeaf(): u32`
- `expandNode(*_, node_idx: u32, actions: []const i32, priors: []const f64, next_player: i8): usize`
- `markTerminal(*_, node_idx: u32)`
- `backpropagate(*_, leaf_idx: u32, value_for_root: f64)`
- `bestActionByVisits(): i32?`、`bestActionByMeanValue(): i32?`
- `rootActionStats(actions_out: []i32, visits_out: []u32, mean_vals_out: []f64): usize`

## 使用示例

```cp
import std from "std";
import { MctsNode, MctsConfig, MctsTree } from "cap:ml";

fn example() {
    let pool: [128]MctsNode = std.mem.zeroes([128]MctsNode);
    if (tree := MctsTree.init(pool[0..], 1, MctsConfig.defaultUct())) {
        let t = tree;
        const actions: [3]i32 = [0, 1, 2];
        const no_priors: [0]f64 = [];
        * = t.expandNode(0, actions[0..], no_priors[0..], 1);
        const leaf = t.selectLeaf();
        t.backpropagate(leaf, 0.8);
        * = t.bestActionByVisits();
    }
}
```
