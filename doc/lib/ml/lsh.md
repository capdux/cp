# `cap:lsh` — 零堆分配局部敏感哈希（LSH）近似最近邻检索库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ml`，导入请使用 `import { ... } from "cap:ml"`。

`cap:lsh` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的局部敏感哈希索引，包含面向余弦相似度的随机超平面 LSH（`CosineLsh` / Sign-Random-Projection）与面向欧氏距离的 $p$-稳定分布 LSH（`EuclideanLsh` / E2LSH，Datar-Immorlica-Indyk-Mirrokni 2004）。内置无全局状态 SplitMix64 + Box-Muller 高斯生成器，保证相同 `seed` 的完全确定性复现。

## 核心特性

- **100% 零堆分配**：随机投影矩阵/偏移、哈希桶头指针数组与链地址节点池均由调用方以切片传入。
- **随机超平面余弦 LSH（`CosineLsh`）**：支持 `num_tables` 张独立哈希表、每表 `1..64` 位超平面签名、精确签名匹配与 `estimatedCosineFromHamming` 余弦估计。
- **$p$-稳定分布欧氏 LSH（`EuclideanLsh`）**：基于 $h_{a,b}(v) = \lfloor (a \cdot v + b) / w \rfloor$（$a \sim \mathcal{N}(0, I), b \sim U[0, w)$）的多投影组合签名，高效聚类与近邻召回。
- **跨表 O(1) 去重检索**：`queryCandidates` 结合调用方提供的 `seen_bitset` 自动清零并去重，杜绝多表命中导致的重复候选 ID。
- **完整防御校验**：拦截非法位数（`0` 或 `> 64`）、非法桶宽（`<= 0` 或 `NaN`）、缓冲不足、条目池耗尽与 `NaN` 向量。

---

## 导出清单

### 常量与结构体

- `LSH_NO_ENTRY: u32 = 0xFFFFFFFF`
- `LshEntry`：`{ item_id: u32, signature: u64, next: u32 }`
- `CosineLsh`：随机超平面余弦 LSH 索引
- `EuclideanLsh`：$p$-稳定分布欧氏距离 LSH 索引

### 核心方法与函数

| 签名 | 说明 |
| :--- | :--- |
| `CosineLsh.init(dim, num_tables, num_bits, num_buckets, seed, planes_buf, heads_buf, entries_buf): CosineLsh?` | 初始化余弦 LSH 索引 |
| `CosineLsh.hashVector(vec, table_idx): u64?` | 计算 `num_bits` 位超平面签名 |
| `CosineLsh.insert(*_, item_id, vec): bool` | 插入全部 `num_tables` 张哈希表 |
| `CosineLsh.queryCandidates(vec, out_ids, seen_bitset): usize?` | 跨表检索去重候选 `item_id` 列表 |
| `estimatedCosineFromHamming(sig_a, sig_b, num_bits): f64` | 通过汉明距离估计余弦相似度 $\cos(\pi \cdot d_H / k)$ |
| `EuclideanLsh.init(dim, num_tables, num_proj, bucket_width, num_buckets, seed, proj_a_buf, proj_b_buf, heads_buf, entries_buf): EuclideanLsh?` | 初始化 E2LSH 索引 |
| `EuclideanLsh.hashVector(vec, table_idx): u64?` | 计算 E2LSH 量化组合签名 |
| `EuclideanLsh.insert(*_, item_id, vec): bool` | 插入全部 `num_tables` 张哈希表 |
| `EuclideanLsh.queryCandidates(vec, out_ids, seen_bitset): usize?` | 跨表检索去重候选 `item_id` 列表 |

---

## 使用示例

```typescript
import std from "std";
import { LshEntry, CosineLsh, estimatedCosineFromHamming } from "cap:ml";

fn lshDemo() {
    let planes_arr: [256]f64 = std.mem.zeroes([256]f64); // 4 tables * 8 bits * 8 dim
    let heads_arr: [128]u32 = std.mem.zeroes([128]u32);  // 4 tables * 32 buckets
    let entries_arr: [64]LshEntry = std.mem.zeroes([64]LshEntry);
    let planes: []f64 = planes_arr[0..];
    let heads: []u32 = heads_arr[0..];
    let entries: []LshEntry = entries_arr[0..];

    let idx = CosineLsh.init(8, 4, 8, 32, 42, &planes, &heads, &entries) ?? return;
    const v0 = [1.0, 0.5, 0.2, 0.1, 0.0, 0.3, 0.4, 0.2];
    const v1 = [1.0, 0.52, 0.19, 0.11, 0.01, 0.29, 0.41, 0.2];
    * = idx.insert(0, v0[0..]);
    * = idx.insert(1, v1[0..]);

    let out_arr: [16]u32 = std.mem.zeroes([16]u32);
    let seen_arr: [8]u8 = std.mem.zeroes([8]u8);
    let out_ids: []u32 = out_arr[0..];
    let seen: []u8 = seen_arr[0..];
    const hits = idx.queryCandidates(v0[0..], &out_ids, &seen);
    const s0 = idx.hashVector(v0[0..], 0) ?? 0;
    const s1 = idx.hashVector(v1[0..], 0) ?? 0;
    const est = estimatedCosineFromHamming(s0, s1, 8);
    * = hits;
    * = est;
}
```
