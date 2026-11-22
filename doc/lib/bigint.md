# CP 标准库：任意精度大整数 (`BigInt`) 与 Web3/EVM 256 位整数 (`BigInt256`) 使用手册 (`cap:bigint`)

> 参考来源：`/data/dev/bun/src/jsc/bindings/JSBigInt.cpp`, `/data/dev/bun/src/jsc/JSBigInt.cp` | 吸收日期：2026-11-22
> **模块路径**: `lib/bigint/`  
> **导入协议**: `import { BigInt, BigIntDivMod, BigInt256, BigInt256DivMod, BigInt256Overflow } from "cap:bigint";`

---

## 一、 架构总览与核心能力

`cap:bigint` 为 CP 提供**双轨高精度整数计算引擎**：

1. **`BigInt`（任意精度有符号大整数）**：
   - **四则与带余除法**：`add`、`sub`、`mul`、`divTrunc`、`rem`、`divMod`（同时返回商与余数）、`mod`（数学向下取整模，正模数下结果恒非负）、`abs`、`neg`。
   - **比较与属性**：`cmp` / `order`、`eq`、`ne`、`lt`、`le`、`gt`、`ge`、`isZero` / `eqZero`、`isPositive`、`isNegative`、`isEven`、`isOdd`、`bitCount`。
   - **位运算与数论密码学**：`bitAnd`、`bitOr`、`bitXor`、`shl`、`shr`、`pow`、`gcd`、`modPow`（平方-乘模幂算法，支持 RSA/DH 大数验算）。
   - **多进制双向转换**：`fromDecimal`/`toDecimal`、`fromHex`/`toHex`/`toHexPrefixed`、`fromBinary`/`toBinary`/`toBinaryPrefixed`（完整兼容 `0x`/`0b`/`0o` 前缀与正负号）。
2. **`BigInt256`（32 字节栈内 Web3/EVM 256 位无符号大整数）**：
   - **回绕算术与溢出检测**：`add`/`sub`/`mul`（模 $2^{256}$）以及 `addOverflow`/`addChecked`、`subOverflow`/`subChecked`、`mulOverflow`/`mulChecked`。
   - **除法、移位与位运算**：`divMod`、`div`、`mod`、`shl`、`shr`、`bitAnd`、`bitOr`、`bitXor`、`bitNot`。
   - **完整进制转换**：`fromHex`/`toHex`/`toHexPrefixed`、`fromDecimal`/`toDecimal`、`fromBinary`/`toBinary`。

---

## 二、 实战代码示例

### 1. 任意精度 `BigInt`：带余除、进制转换、GCD 与 RSA 模幂

```typescript
import { BigInt } from "cap:bigint";

pub fn demoBigInt(): !void {
    let a = BigInt.fromHex("-0x1a2b3c4d5e6f708090a0b0c0d0e0f0")?;
    defer a.deinit();
    const hex_str = a.toHexPrefixed(); // "-0x1a2b3c4d5e6f708090a0b0c0d0e0f0"

    let x = BigInt.fromDecimal("-17")?;
    defer x.deinit();
    let m = BigInt.fromDecimal("5")?;
    defer m.deinit();

    let dm = x.divMod(&m)?;
    defer dm.deinit(); // quotient = -3, remainder = -2

    let floored = x.mod(&m)?;
    defer floored.deinit(); // 3

    // RSA 模幂验算: (65^17 mod 3233)^2753 mod 3233 == 65
    let msg = BigInt.fromUint(65)?;
    defer msg.deinit();
    let e = BigInt.fromUint(17)?;
    defer e.deinit();
    let d = BigInt.fromUint(2753)?;
    defer d.deinit();
    let n = BigInt.fromUint(3233)?;
    defer n.deinit();

    let cipher = msg.modPow(&e, &n)?;
    defer cipher.deinit(); // 2790
    let plain = cipher.modPow(&d, &n)?;
    defer plain.deinit();  // 65
}
```

### 2. `BigInt256`：带溢出检测的 Web3 256 位算术

```typescript
import { BigInt256 } from "cap:bigint";

pub fn demoBigInt256(): !void {
    const val = BigInt256.fromDecimal("1000000000000000000000000")?;
    const dm = val.divMod(BigInt256.fromU64(97))?;
    const shifted = BigInt256.one().shl(200);

    const ov = BigInt256.max().addOverflow(BigInt256.one());
    // ov.overflow == true, ov.value.isZero() == true
}
```

---

## 三、 上游行为契约与数值边界注意事项

1. **`BigInt` 堆内存确定性释放与 `divTrunc`/`mod` 符号语义**：
   - `BigInt` 底层持有动态多精度肢体数组，创建或运算返回的 `BigInt` 与 `BigIntDivMod` 均须配对 `defer val.deinit();`；`divTrunc`/`rem` 向零截断（与 C/JS 一致），`mod` 按数学欧几里得取模（正模数下结果恒非负）。
2. **`BigInt256` 零堆回绕 vs 检查算术**：
   - `BigInt256` 为 32 字节纯值类型（零堆分配），`add`/`sub`/`mul` 默认按 EVM 语义模 $2^{256}$ 回绕；金融与账本计算务必使用 `addChecked`/`subChecked`/`mulChecked` 或 `addOverflow` 拦截溢出。
3. **除零与负指数防护**：
   - `divTrunc`/`divMod`/`mod` 遇除数为 0 时安全返回错误；`fromString` 自动识别 `0x`/`0b`/`0o` 前缀并在空串或非法数字字符时返回 `InvalidCharacter`。
