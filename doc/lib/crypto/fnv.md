# `cap:fnv` — FNV-1 / FNV-1a (32/64/128-bit) 与 DJB2 / SDBM / Jenkins OAAT 哈希套件

> [!NOTE]
> 本子模块已归并至统一主包 `cap:crypto`，导入请使用 `import { ... } from "cap:crypto"`。

纯 CP、零堆分配的非加密哈希标准库，完整实现 IETF `draft-eastlake-fnv` 规范的 32/64/128 位 FNV-1 与 FNV-1a 流式/一次性哈希、官方 XOR-folding 位宽折叠，以及 DJB2、DJB2-XOR、SDBM、Bob Jenkins One-at-a-Time 经典轻量哈希。

## 导入

```cp
import {
    FNV32_OFFSET_BASIS,
    FNV32_PRIME,
    FNV64_OFFSET_BASIS,
    FNV64_PRIME,
    FNV128_OFFSET_BASIS,
    FNV128_PRIME,
    Fnv1a32,
    Fnv1a64,
    Fnv1a128,
    Fnv1_32,
    Fnv1_64,
    Fnv1_128,
    fnv1a32,
    fnv1a64,
    fnv1a128,
    fnv1_32,
    fnv1_64,
    fnv1_128,
    fnvXorFold32,
    djb2,
    djb2Xor,
    sdbm,
    jenkinsOaat,
} from "cap:crypto";
```

## 核心 API

- **流式 FNV 哈希器**：`Fnv1a32` / `Fnv1a64` / `Fnv1a128` / `Fnv1_32` / `Fnv1_64` / `Fnv1_128`（均提供 `init()`, `update(*_, data)`, `updateByte(*_, b)`, `finish()`）。
- **一次性 FNV 与 XOR-Folding**：
  - `fnv1a32` / `fnv1a64` / `fnv1a128`、`fnv1_32` / `fnv1_64` / `fnv1_128`
  - `fnvXorFold32(h64: u64, bits: u8): u64?` — 将 64 位哈希按 FNV 官方规范折叠至 `1..63` 位。
- **经典轻量哈希**：`djb2(data): u32`、`djb2Xor(data): u32`、`sdbm(data): u32`、`jenkinsOaat(data): u32`。

## 示例

```cp
import { Fnv1a64, fnv1a64, jenkinsOaat } from "cap:crypto";

fn hashStream(a: []const u8, b: []const u8): u64 {
    let h = Fnv1a64.init();
    h.update(a);
    h.update(b);
    return h.finish();
}
```
