# @cap/hyperloglog

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:hyperloglog` ([https://github.com/jedisct1/rust-hyperloglog](https://github.com/jedisct1/rust-hyperloglog))

零堆分配的 256 字节 HyperLogLog 概率基数估算与分布式合并引擎 — 洁净室 MIT 实现，对标 Rust `hyperloglog` 与 Redis `PFADD`/`PFCOUNT`/`PFMERGE`，仅用 256 字节栈空间即可高效估算十万/百万级 UV 去重计数。

Zero-allocation 256-byte HyperLogLog cardinality estimator with LinearCounting correction and distributed merge for CP — clean-room MIT parity with Rust `hyperloglog` and Redis `PFADD`/`PFCOUNT`/`PFMERGE`.

## 安装 / Install

```bash
cap add @cap/hyperloglog
```

## API 参考 / API Reference

- **`HyperLogLog256`**（`p = 8`, `m = 256` 寄存器，256B 固定栈占用）：
  - `init()`, `clear(*_)`, `registerCount() -> usize`
  - `add(*_, item: string)`, `addU64(*_, val: u64)`
  - `count() -> u64`（调和平均 + LinearCounting 小基数纠偏）
  - `merge(*_, other: HyperLogLog256)`（无损分布式 UV 合并）
  - `relativeError() -> f64`（`1.04 / sqrt(256) = 0.065`）

## 使用示例 / Usage

```cp
import { HyperLogLog256 } from "@cap/hyperloglog";

let hll = HyperLogLog256.init();
hll.add("user:1001");
hll.add("user:1002");
hll.add("user:1001"); // duplicate ignored
const uv = hll.count(); // ~2
```

## License

MIT
