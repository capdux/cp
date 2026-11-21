# `cap:bit_ops`

纯 CP 零开销底层位运算、位域提取/插入、2 的幂对齐与置位迭代器库，吸收自 `/data/dev/bun/src/meta/bits.cp`。

## 导出符号

- 位操作：`testBit64`, `setBit64`, `clearBit64`, `toggleBit64`, `intersects64`, `containsAll64`, `maskOut64`
- 位域：`extractBits64(val, shift, width)`, `insertBits64(val, field, shift, width)`
- 计数与最低位：`popcount64`, `clz64`, `ctz64`, `clearLowestSetBit`, `isolateLowestSetBit`
- 对齐与 2 的幂：`isPowerOfTwo`, `nextPowerOfTwo`, `alignUp`, `alignDown`
- 迭代器：`BitSetIter64`（按升序遍历 64 位掩码中所有置位索引）
