# `cap:attention` — Transformer Attention, KV-Cache & Token Sampler

## 1. 概述

`cap:attention`（`cap.attention`）是 CP 语言纯原生实现的工业级大语言模型推理核心算子库，提供数值稳定的 Scaled Dot-Product Attention (MHA)、Grouped-Query Attention (GQA)、支持 Speculative Decoding 回滚的自回归 `KvCache`，以及支持 Temperature、Repetition Penalty、Top-K 与 Top-P (Nucleus) 的确定性 Token 采样器。

## 2. 核心类型

- **`AttnError`**: 统一错误集合（`InvalidDimension`, `HeadMismatch`, `InvalidStartPos`, `EmptyInput`, `CacheFull`, `InvalidLayer`, `InvalidTemperature`, `InvalidTopK`, `InvalidTopP`, `OutOfMemory`）。
- **`PagedKvSlot`**: 单步追加返回的槽位元信息（`layer`, `seq_offset`, `num_tokens`）。
- **`KvCache`**: 多层自回归 KV 缓存容器，按 `[layer][seq_pos][kv_head][head_dim]` 布局存储，支持 `appendStep`、`truncateTo` 回滚与 `reset`。
- **`KV_INVALID_BLOCK` / `KvBlockMeta` / `KvAllocStats` / `PagedKvAllocator` / `BlockTable`** (`kv_cache.cp`)：零堆分配 PagedAttention 物理块管理器，支持系统提示词前缀哈希零拷贝共享、释放后缓存复用与 LRU 驱逐、Beam Search `retainBlock` + 引用计数 CoW（共享态分配新块，独占态零拷贝原地返回）以及 `BlockTable` 逻辑到物理槽位映射。
- **`SamplingConfig`**: 采样配置（`temperature`, `top_k`, `top_p`, `repetition_penalty`, `seed`）。
- **`RopeConfig` / RoPE / RMSNorm / LayerNorm / SwiGLU / GeLU** (`rope.cp`)：零堆分配 Transformer 基础推理核，支持 NTK-Aware 长上下文缩放的 RoPE 旋转位置编码、RMSNorm/LayerNorm 归一化与 SiLU/GeLU/SwiGLU 激活函数。

## 3. API 速查

| 符号 | 签名概要 | 说明 |
| --- | --- | --- |
| `scaledDotProductAttention` | `(alloc, q, k, v, seq_q, seq_k, head_dim, causal, start_pos): AttnError![]f32` | 数值稳定 Softmax 注意力，支持因果掩码与 `start_pos` 偏移 |
| `groupedQueryAttention` | `(alloc, q, k_cache, v_cache, num_q_heads, num_kv_heads, seq_k, head_dim): AttnError![]f32` | 分组查询注意力，校验 `num_q_heads % num_kv_heads == 0` |
| `KvCache.init` | `(alloc, num_layers, num_kv_heads, head_dim, max_seq_len): AttnError!KvCache` | 初始化多层 KV-Cache |
| `KvCache.deinit` | `(*_)` | 释放内部缓冲区 |
| `KvCache.appendStep` | `(*_, layer, k_vec, v_vec): AttnError!PagedKvSlot` | 追加单步 KV 向量 |
| `KvCache.seqLen` | `(layer): usize` | 查询指定层当前序列长度 |
| `KvCache.keysSlice` / `valuesSlice` | `(layer): []const f32` | 获取指定层当前有效 K/V 切片 |
| `KvCache.truncateTo` | `(*_, new_len)` | 回滚所有层至 `new_len`（用于 Speculative Decoding） |
| `KvCache.reset` | `(*_)` | 清空所有层缓存长度 |
| `sampleGreedy` | `(logits): AttnError!usize` | 贪心 Argmax 采样 |
| `sampleToken` | `(alloc, logits, recent_tokens, config): AttnError!usize` | 支持 RepPenalty + Temp + Top-K + Top-P + SplitMix64 采样 |
| `PagedKvAllocator.init` | `(blocks: *[]KvBlockMeta, block_size: u16): PagedKvAllocator?` | 零堆分配初始化固定物理块池（校验 `blocks.len > 0` 与 `block_size > 0`） |
| `PagedKvAllocator.allocateOrSharePrefix` | `(*_, prefix_hash: u64, tick: u64): u32?` | 命中相同 `prefix_hash` 直接 `ref_count += 1` 零拷贝共享；否则分配空闲块或 LRU 驱逐 `ref_count == 0` 缓存块 |
| `PagedKvAllocator.allocateFresh` | `(*_, tick: u64): u32?` | 分配独占无哈希新块（`ref_count = 1`） |
| `PagedKvAllocator.retainBlock` / `releaseBlock` | `(*_, block_id: u32, tick: u64): bool` | 引用计数递增（Beam Search 共享）与递减（归零后带哈希块自动保留为可复用缓存） |
| `PagedKvAllocator.copyOnWrite` | `(*_, block_id: u32, tick: u64): u32?` | `ref_count > 1` 时分配新块并递增 `cow_copies`；`ref_count == 1` 时零拷贝原地返回 `block_id` |
| `PagedKvAllocator.stats` | `(): KvAllocStats` | 返回总块数、空闲块、可驱逐缓存块、前缀命中/未命中与 CoW 次数 |
| `BlockTable.init` / `appendBlock` / `mapTokenToPhys` | `(phys_blocks, block_size)` / `(phys_id)` / `(logical_pos): ([2]u32)?` | 序列逻辑块表与 `[phys_block_id, slot_offset]` 映射 |
| `RopeConfig.default` | `(head_dim: usize): RopeConfig` | 构造默认 RoPE 配置（`base_theta = 10000.0`, `ntk_scale = 1.0`） |
| `ropePrecomputeTables` | `(cfg, max_seq_len, cos_out, sin_out): bool` | 预计算 `[max_seq_len][head_dim / 2]` RoPE cos/sin 查找表（支持 NTK 缩放） |
| `ropeApplyInPlace` | `(vec_inout, pos, head_dim, cos_table, sin_table): bool` | 对单头 Q/K 向量执行严格保范数的半切分旋转位置编码 |
| `rmsNorm` / `layerNorm` | `(x, weight, eps, out): bool` / `(x, gamma, beta, eps, out): bool` | 零堆分配 RMSNorm（支持原地与空权重）与 LayerNorm 归一化核 |
| `silu` / `geluTanh` / `swigluForward` | `(x): f64` / `(x): f64` / `(gate, up, out): bool` | 现代 LLM FFN 激活核（`swigluForward`: `out[i] = silu(gate[i]) * up[i]`） |

## 4. 示例

```cp
import { KvCache, groupedQueryAttention, SamplingConfig, sampleToken } from "cap:attention";

let cache = try KvCache.init(alloc, 2, 2, 4, 16);
defer cache.deinit();

_ = try cache.appendStep(0, k_step, v_step);
const ctx = try groupedQueryAttention(
    alloc,
    q_step,
    cache.keysSlice(0),
    cache.valuesSlice(0),
    8,
    2,
    cache.seqLen(0),
    4,
);
defer alloc.free(ctx);

const tok = try sampleToken(alloc, logits, &[_]usize{1, 4}, SamplingConfig{
    .temperature = 0.8,
    .top_k = 40,
    .top_p = 0.9,
    .repetition_penalty = 1.1,
    .seed = 12345,
});
```

### PagedAttention 前缀共享、CoW 与逻辑块表映射示例 (`kv_cache.cp`)

```cp
import {
    KV_INVALID_BLOCK,
    KvBlockMeta,
    PagedKvAllocator,
    BlockTable,
} from "cap:attention";

pub fn demoPagedKvPrefixSharing() {
    let pool_buf: [16]KvBlockMeta = [KvBlockMeta{}; 16];
    let pool: []KvBlockMeta = pool_buf[0..];
    if (alloc_init := PagedKvAllocator.init(pool, 16)) {
        let kv_alloc = alloc_init;
        const sys_prefix_hash: u64 = 0xDEADBEEF0001;

        // 请求 1 与请求 2 零拷贝共享系统提示词前缀块
        const b1 = kv_alloc.allocateOrSharePrefix(sys_prefix_hash, 10) ?? KV_INVALID_BLOCK;
        const b2 = kv_alloc.allocateOrSharePrefix(sys_prefix_hash, 12) ?? KV_INVALID_BLOCK; // prefix_hits += 1

        // 请求 2 修改块内容触发 CoW，自动分配独占新块
        const b2_cow = kv_alloc.copyOnWrite(b2, 15) ?? KV_INVALID_BLOCK;

        let phys_buf: [8]u32 = [0; 8];
        let phys: []u32 = phys_buf[0..];
        if (bt_init := BlockTable.init(phys, 16)) {
            let bt = bt_init;
            * = bt.appendBlock(b1);
            * = bt.appendBlock(b2_cow);
            if (mapped := bt.mapTokenToPhys(19)) {
                // mapped == [b2_cow, 3]
                * = mapped;
            }
        }
    }
}
```

### RoPE（含 NTK 缩放）、RMSNorm 与 SwiGLU 推理核示例 (`rope.cp`)

```cp
import {
    RopeConfig,
    ropePrecomputeTables,
    ropeApplyInPlace,
    rmsNorm,
    swigluForward,
} from "cap:attention";

pub fn demoTransformerKernels() {
    // 1. 预计算 NTK-Aware RoPE 表并对 Q 向量应用位置编码
    const cfg = RopeConfig{ head_dim: 4, base_theta: 10000.0, ntk_scale: 2.0 };
    let cos_buf: [16]f64 = [0.0; 16]; // max_seq_len=8 * (head_dim/2)=2
    let sin_buf: [16]f64 = [0.0; 16];
    let cos_s: []f64 = cos_buf[0..];
    let sin_s: []f64 = sin_buf[0..];
    * = ropePrecomputeTables(cfg, 8, cos_s, sin_s);

    let q_buf: [4]f64 = [1.0, -0.5, 0.25, 0.75];
    let q_s: []f64 = q_buf[0..];
    * = ropeApplyInPlace(q_s, 5, 4, cos_buf[0..], sin_buf[0..]);

    // 2. RMSNorm 与 SwiGLU FFN
    let norm_buf: [4]f64 = [0.0; 4];
    let ffn_buf: [4]f64 = [0.0; 4];
    let norm_s: []f64 = norm_buf[0..];
    let ffn_s: []f64 = ffn_buf[0..];
    const empty_w: []const f64 = q_buf[0..0];
    * = rmsNorm(q_buf[0..], empty_w, 0.00001, norm_s);
    * = swigluForward(norm_buf[0..], q_buf[0..], ffn_s);
}
```
