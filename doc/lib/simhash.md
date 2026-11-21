# `cap:simhash` — 64-bit Charikar SimHash 近似去重指纹与 4×16-bit LSH 分段库

纯 CP、零堆分配的 64-bit Moses Charikar SimHash 局部敏感哈希标准库，提供 FNV-1a 64-bit 特征哈希、栈上 64 维权重累加器 `SimHasher`、零拷贝分词与滑动 `k`-shingle 切片、汉明距离/相似度以及基于鸽巢原理的 4×16-bit LSH 分段检索键。

## 导入

```cp
import {
    SIMHASH_BITS,
    SIMHASH_DEFAULT_THRESHOLD,
    fnv1a64,
    SimHasher,
    simhashText,
    simhashShingles,
    hammingDistance,
    simhashSimilarity,
    isNearDuplicate,
    simhashBands4,
    sharesAnyBand4,
} from "cap:simhash";
```

## 核心 API

- **协议常量与特征哈希**：
  - `SIMHASH_BITS = 64`、`SIMHASH_DEFAULT_THRESHOLD = 3`
  - `fnv1a64(data: []const u8): u64`
- **流式权重累加器 `SimHasher`**：
  - `SimHasher.init(): SimHasher`
  - `addHash(*_, hash: u64, weight: i64)`、`addToken(*_, token: []const u8, weight: i64)`
  - `addTextWords(*_, text: []const u8)`（ASCII 字母数字分词 + 小写归一化）
  - `addCharShingles(*_, text: []const u8, k: usize)`（滑动 `k`-字节窗口，适合中英文无空格文本）
  - `finish(): u64`、`reset(*_)`
- **快捷指纹、距离与 4×16-bit LSH 分段**：
  - `simhashText(text: []const u8): u64`、`simhashShingles(text: []const u8, k: usize): u64`
  - `hammingDistance(a: u64, b: u64): u8`、`simhashSimilarity(a: u64, b: u64): f64`、`isNearDuplicate(a: u64, b: u64, max_dist: u8): bool`
  - `simhashBands4(fp: u64): [4]u16`、`sharesAnyBand4(a: u64, b: u64): bool`（当 `hammingDistance(a, b) <= 3` 时恒为 `true`）

## 示例

```cp
import { simhashText, isNearDuplicate, SIMHASH_DEFAULT_THRESHOLD } from "cap:simhash";

fn checkNearDup(doc_a: []const u8, doc_b: []const u8): bool {
    const fa = simhashText(doc_a);
    const fb = simhashText(doc_b);
    return isNearDuplicate(fa, fb, SIMHASH_DEFAULT_THRESHOLD);
}
```
