# `cap:elias` — 通用前缀整数编码与位流零分配套件

> [!NOTE]
> 本子模块已归并至统一主包 `cap:codec`，导入请使用 `import { ... } from "cap:codec"`。

> **模块路径**: `lib/elias/`  
> **导入协议**: `import { BitWriter, BitReader, writeEliasGamma, readEliasGamma, writeEliasDelta, readEliasDelta, writeEliasOmega, readEliasOmega, writeExpGolombU, readExpGolombU, writeExpGolombS, readExpGolombS, writeRice, readRice, writeFibonacci, readFibonacci } from "cap:codec";`  
> **动态挂载**: `import cap from "cap";` -> `cap.elias.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:elias` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的通用前缀整数编码与 MSB-first 位流编解码套件：

1. **MSB-first 位流读写器 (`BitWriter` / `BitReader`)**：
   - 支持单比特、任意 `0..64` 位宽整数与批量零比特写入，自动清零新字节避免脏内存污染，严格检查缓冲边界。
2. **Elias 通用前缀码 (`n >= 1`)**：
   - 完整实现 Elias Gamma、Elias Delta 与 Elias Omega（递归分组，零堆分配），支持高达 64-bit 正整数编解码与前导零防溢出保护。
3. **H.264 / HEVC Exp-Golomb (`ue(v)` / `se(v)`)**：
   - 支持无符号 `0..2^64-2` 与有符号交错映射（`0->0, 1->1, -1->2, 2->3, -2->4...`）。
4. **Golomb-Rice (`k = 0..62`) & Fibonacci Zeckendorf (`n >= 1`)**：
   - Golomb-Rice 支持一元商 + `k`-bit 余数；Fibonacci 编码采用 92 项 Zeckendorf 表示与自同步 `11` 终止符（`1->"11"`, `2->"011"`, `10->"010011"`）。

---

## 二、 核心 API 速查表 (`lib/elias/elias.cp`)

| 类别 | 符号 / 函数 | 签名与说明 |
| :--- | :--- | :--- |
| 位流读写 | `BitWriter` | `init(buf)`, `writeBit(b: u1)`, `writeBits(val, n)`, `writeZeros(n)`, `bytesUsed()`, `flush()` |
| 位流读写 | `BitReader` | `init(data)`, `readBit(): u1?`, `readBits(n): u64?`, `bitsRemaining()` |
| Elias 码 | Gamma / Delta / Omega | `writeEliasGamma` / `readEliasGamma`, `writeEliasDelta` / `readEliasDelta`, `writeEliasOmega` / `readEliasOmega` |
| Exp-Golomb | `ue(v)` / `se(v)` | `writeExpGolombU` / `readExpGolombU`, `writeExpGolombS` / `readExpGolombS` |
| Golomb-Rice | Rice (`k = 0..62`) | `writeRice(bw, v, k)` / `readRice(br, k)` |
| Fibonacci | Zeckendorf (`11` 终止) | `writeFibonacci(bw, n)` / `readFibonacci(br)` |

---

## 三、 使用示例

```typescript
import { BitWriter, BitReader, writeEliasDelta, readEliasDelta, writeExpGolombS, readExpGolombS } from "cap:codec";

fn demo() {
    let buf: [64]u8 = [0; 64];
    let bw = BitWriter.init(buf[0..]);
    * = writeEliasDelta(*bw, 1024);
    * = writeExpGolombS(*bw, -42);

    let br = BitReader.init(bw.flush());
    const a = readEliasDelta(*br);
    const b = readExpGolombS(*br);
    * = a;
    * = b;
}
```
