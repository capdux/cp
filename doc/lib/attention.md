# `cap:attention` — Transformer Attention, KV-Cache & Token Sampler

## 1. 概述

`cap:attention`（`cap.attention`）是 CP 语言纯原生实现的工业级大语言模型推理核心算子库，提供数值稳定的 Scaled Dot-Product Attention (MHA)、Grouped-Query Attention (GQA)、支持 Speculative Decoding 回滚的自回归 `KvCache`，以及支持 Temperature、Repetition Penalty、Top-K 与 Top-P (Nucleus) 的确定性 Token 采样器。

## 2. 核心类型

- **`AttnError`**: 统一错误集合（`InvalidDimension`, `HeadMismatch`, `InvalidStartPos`, `EmptyInput`, `CacheFull`, `InvalidLayer`, `InvalidTemperature`, `InvalidTopK`, `InvalidTopP`, `OutOfMemory`）。
- **`PagedKvSlot`**: 单步追加返回的槽位元信息（`layer`, `seq_offset`, `num_tokens`）。
- **`KvCache`**: 多层自回归 KV 缓存容器，按 `[layer][seq_pos][kv_head][head_dim]` 布局存储，支持 `appendStep`、`truncateTo` 回滚与 `reset`。
- **`SamplingConfig`**: 采样配置（`temperature`, `top_k`, `top_p`, `repetition_penalty`, `seed`）。

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
