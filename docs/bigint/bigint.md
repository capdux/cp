# CP 标准库：任意精度大整数 (`BigInt`) 与 Web3/EVM 256 位整数 (`BigInt256`) 使用手册 (`cap:bigint`)

## 一、 架构总览与设计哲学

`cap:bigint` 为 CP 语言提供**双轨高精度整数计算引擎**，兼顾通用任意精度数学/密码学计算与 Web3/EVM/金融清算场景的极致定长 256 位性能：

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│                         cap:bigint 双轨大整数引擎                             │
│                                                                              │
│  ┌────────────────────────────────────┐  ┌────────────────────────────────┐  │
│  │ BigInt (任意精度有符号整数)         │  │ BigInt256 (定长 256 位无符号)   │  │
│  │  • 底层：std.math.big.int.Managed  │  │  • 底层：4×u64 栈内值类型 (32B) │  │
│  │  • 支持任意进制字符串解析/输出      │  │  • 原生硬件级 u256 算术         │  │
│  │  • add / sub / mul / divTrunc / rem│  │  • 零堆分配 (fromHex/add/sub/  │  │
│  │  • order / eq / isNegative / bits  │  │    mul/cmp/eq)                 │  │
│  │  • 必须配对 defer x.deinit()       │  │  • 完美契合 EVM uint256 语义   │  │
│  └────────────────────────────────────┘  └────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## 二、 核心 API 详解与代码示例

### 1. 任意精度有符号大整数 (`BigInt`)

支持从十进制/十六进制/二进制字符串或 `i64`/`u64` 构造任意位宽大整数；由于内部按需分配 limb 数组，务必配对 `defer x.deinit()` 确定性释放内存：

```typescript
import { BigInt } from "cap:bigint";

pub fn demoBigInt(): !void {
    let a = BigInt.fromString("1234567890123456789012345678901234567890", 10)?;
    defer a.deinit();

    let b = BigInt.fromString("9876543210987654321098765432109876543210", 10)?;
    defer b.deinit();

    let sum = a.add(&b)?;
    defer sum.deinit();
    const sum_str = sum.toString(10); // "11111111101111111110111111111011111111100"

    let diff = a.sub(&b)?;
    defer diff.deinit();
    const neg = diff.isNegative();    // true
    const cmp = a.order(&b);          // -1 (lt), 0 (eq), 1 (gt)

    let x = BigInt.fromUint(1000000)?;
    defer x.deinit();
    let y = BigInt.fromUint(3000)?;
    defer y.deinit();

    let q = x.divTrunc(&y)?;
    defer q.deinit();
    let r = x.rem(&y)?;
    defer r.deinit();
    const bits = x.bitCount();        // 20
}
```

### 2. 零堆分配 256 位定长整数 (`BigInt256`)

专为区块链智能合约、哈希值运算与高频定点金额计算设计，仅占 32 字节（`[4]u64`），全程栈上值传递，采用模 $2^{256}$ 回绕算术（`+%` / `-%` / `*%`）：

```typescript
import { BigInt256 } from "cap:bigint";

pub fn demoBigInt256(): !void {
    const base = BigInt256.fromHex("0xffffffffffffffff")?;
    const one = BigInt256.fromU64(1);
    const next = base.add(one);

    const hex = next.toHex();     // "10000000000000000"
    const ord = base.cmp(next);   // -1
    const equal = next.eq(BigInt256.fromHex("0x10000000000000000")?); // true
}
```

---

## 三、 与 JS / Bun `BigInt` 对照表

| 特性 | JS / Bun `BigInt` (`123n`) | CP `BigInt` (`cap:bigint`) | CP `BigInt256` (`cap:bigint`) |
|---|---|---|---|
| 内存模型 | GC 堆分配对象 | 显式 `defer deinit()` 确定性释放 | 32 字节纯栈值类型，零堆分配 |
| 位宽上限 | 任意精度 | 任意精度 | 固定 256 位（4×`u64`） |
| 错误处理 | `RangeError` / `SyntaxError` 异常 | 强类型错误联合 `?` | `fromHex` 返回 `!BigInt256`，算术无异常 |
| 典型场景 | 通用大数、密码学、高精度数学 | RSA/大数分解、任意位宽精确算术 | EVM `uint256`、代币精度、哈希比较 |
