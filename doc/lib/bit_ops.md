# `cap:bit_ops`

> 参考来源：/data/dev/bun/src/meta/bits.cp | 吸收日期：2026-11-22

纯 CP 零开销底层位运算、位域提取/插入、2 的幂对齐与置位迭代器库。

## 导出符号

- 位操作：`testBit64`, `setBit64`, `clearBit64`, `toggleBit64`, `intersects64`, `containsAll64`, `maskOut64`
- 位域：`extractBits64(val, shift, width)`, `insertBits64(val, field, shift, width)`
- 计数与最低位：`popcount64`, `clz64`, `ctz64`, `clearLowestSetBit`, `isolateLowestSetBit`
- 对齐与 2 的幂：`isPowerOfTwo`, `nextPowerOfTwo`, `alignUp`, `alignDown`
- 迭代器：`BitSetIter64`（按升序遍历 64 位掩码中所有置位索引）

## 上游行为契约与位运算边界

1. **硬件指令直降与安全移位防护**：
   - `popcount64`、`clz64`、`ctz64` 直降单条 CPU 指令（`POPCNT` / `LZCNT` / `TZCNT`）；对 `0` 输入 `clz64(0) == 64`、`ctz64(0) == 64`，`extractBits64` / `insertBits64` 在 `width == 0` 或 `shift >= 64` 时安全处理不触发未定义移位溢出。
2. **对齐与 2 的幂边界**：
   - `isPowerOfTwo(0) == false`；`alignUp` 与 `alignDown` 要求对齐粒度 `alignment` 为 2 的幂；`nextPowerOfTwo` 适用于环形缓冲与哈希表容量向上取整。
3. **稀疏掩码 O(k) 迭代 (`BitSetIter64`)**：
   - 基于 `ctz64` + `clearLowestSetBit` 仅按实际置位数 $k$ 次迭代，跳过所有 0 位，极适合技能掩码、位图索引与事件集合扫描。
