# @cap/cuckoo-filter

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cuckoofilter` ([https://github.com/seiflotfy/rust-cuckoofilter](https://github.com/seiflotfy/rust-cuckoofilter))（原库名 `cuckoofilter` → CP 包名 `@cap/cuckoo-filter`）

Zero-allocation CoNEXT'14 Cuckoo Filter (`256` buckets × `4` slots = `1024` 16-bit fingerprints) with O(1) `insert`, `contains`, and safe `delete` for CP — clean-room MIT parity with Fan et al. (CoNEXT'14) and Rust `cuckoofilter`.

零堆分配的 CP 布谷鸟过滤器，相比传统 Bloom Filter 支持 O(1) 安全删除（`delete`）且在高负载下空间效率更高，适用于缓存穿透拦截、流式去重与网络路由表（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/cuckoo-filter
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `CuckooFilter.init` | `() -> CuckooFilter` | 初始化空过滤器（1024 个 16-bit 非零指纹槽） |
| `insert` | `(*_, data: []const u8) -> bool` | 插入指纹（支持最多 500 次布谷鸟踢出重排） |
| `contains` | `(data: []const u8) -> bool` | O(1) 成员查询（零假阴性） |
| `delete` | `(*_, data: []const u8) -> bool` | O(1) 安全删除单个匹配指纹 |
| `len` / `capacity` / `isEmpty` / `loadFactor` / `clear` | 状态与重置 | 长度、负载率查询与重置 |

## License

MIT
