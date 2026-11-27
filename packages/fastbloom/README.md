# @cap/bloom

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:fastbloom` ([https://github.com/tomtomwombat/fastbloom](https://github.com/tomtomwombat/fastbloom))（原库名 `fastbloom` → CP 包名 `@cap/bloom`）

Zero-allocation Kirsch-Mitzenmacher double-hashing Bloom filters (`BloomFilter1024` / `BloomFilter4096`) for CP — clean-room MIT parity with Rust `bloomfilter` / `fastbloom` and npm `bloom-filters`.

纯 CP 零堆分配高性能布隆过滤器，采用 Kirsch-Mitzenmacher 双哈希技术（FNV-1a + 独立种子 SplitMix64），适用于缓存穿透防护、URL 去重、日志/安全黑名单与分布式多节点聚合。

## Install / 安装

```bash
cap add @cap/bloom
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `BloomFilter1024` / `BloomFilter4096` | `init(num_hashes: u8)` | 1024-bit (16×`u64`) and 4096-bit (64×`u64`) Bloom filters (`k` clamped to `1..16`) |
| `insert` / `mightContain` | `(item: string)` | Zero-false-negative string insertion & membership test |
| `insertU64` / `mightContainU64` | `(val: u64)` | Fast integer key insertion & membership test |
| `bitCount` / `setBits` / `itemCount` / `clear` | `()` | Capacity, popcount, item counter & reset |
| `unionWith` | `(*_, other)` | Bitwise-OR merge of two compatible Bloom filters |
| `estimatedFpr` | `() -> f64` | Theoretical false-positive rate $(1 - e^{-kn/m})^k$ |

## Usage / 使用示例

```cp
import { BloomFilter4096 } from "@cap/bloom";

let bf = BloomFilter4096.init(6 as u8);
bf.insert("https://cap-lang.org");
const hit = bf.mightContain("https://cap-lang.org"); // true
const fpr = bf.estimatedFpr();
```
