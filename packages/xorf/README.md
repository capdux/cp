[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# xorf

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:xorf` ([https://github.com/ayazhafiz/xorf](https://github.com/ayazhafiz/xorf))（原库名 `xorf` → CP 包名 `xorf`）

零堆分配的 CP Xor8 静态指纹过滤器 — 洁净室 MIT 实现，对标 Graf & Lemire ACM JEA 2020 论文与 Rust `xorf` crate，比 Bloom Filter 节省 ~30% 空间，单次查询仅 3 次查表异或 `B[h0] ^ B[h1] ^ B[h2] == fp`，0% 假阴性，理论假阳性率仅 1/256（~0.39%）。

Zero-allocation Xor8 static fingerprint filter for CP — clean-room MIT parity with Rust `xorf` (Graf & Lemire, ACM JEA 2020).

## 安装 / Install

```bash
cap add xorf
```

## API 参考 / API Reference

- **`Xor8Filter`**（最多 256 键，最多 384 字节指纹数组）：
  - `Xor8Filter.build(keys: []const u64) -> Xor8Filter`（自动去重 + Peeling 剥离构建 + 最多 16 次换 seed 重试）
  - `contains(key: u64) -> bool`（O(1) 3 次查表异或）
  - `len() -> usize`、`fingerprintBytes() -> usize`、`isValid() -> bool`

## 使用示例 / Usage

```cp
import { Xor8Filter } from "xorf";

let keys: [4]u64 = [101, 202, 303, 404];
const filter = Xor8Filter.build(keys[0..4]);
const hit = filter.contains(202 as u64); // true (0% false negatives)
```

## License

MIT
