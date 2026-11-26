# @cap/reed-solomon

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:reed-solomon-erasure` ([https://github.com/darrenldl/reed-solomon-erasure](https://github.com/darrenldl/reed-solomon-erasure))（原库名 `reed-solomon-erasure` → CP 包名 `@cap/reed-solomon`）

Zero-allocation GF(2^8) Cauchy Reed-Solomon erasure coding and FEC reconstruction engine for CP — clean-room MIT parity with Rust `reed-solomon-erasure`.

零堆分配的 CP 里德-所罗门纠删码与前向纠错（FEC）引擎，基于标准本原多项式 `0x11D`（`x^8 + x^4 + x^3 + x^2 + 1`）的 GF(2^8) 有限域算术、必然可逆的柯西系统编码矩阵与 GF(2^8) 高斯-约当消元恢复，适用于分布式对象存储、RAID6、QR 码与弱网流媒体抗丢包（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/reed-solomon
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `gfMul` / `gfPow` / `gfInv` | GF(2^8) 有限域算术 | 基于 `0x11D` 本原多项式的精确乘法、幂与逆元 |
| `RsCodec.init` | `(data_shards: usize, parity_shards: usize) -> RsCodec` | 预生成柯西校验矩阵（支持最多 16 数据片 + 8 校验片） |
| `encodeByte` | `(data: []const u8, parity_out: []u8) -> bool` | 生成全部校验分片字节 |
| `verifyByte` | `(data: []const u8, parity: []const u8) -> bool` | 校验数据与校验分片一致性 |
| `reconstructByte` | `(shards: []u8, present: []const bool) -> bool` | GF(2^8) 高斯消元恢复任意 `<= parity_shards` 个丢失分片 |
| `dataShards` / `parityShards` / `totalShards` | `() -> usize` | 分片配置查询 |

## License

MIT
