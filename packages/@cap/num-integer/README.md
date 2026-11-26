# @cap/num-integer

零堆分配的 CP 整数数论与精确除法引擎 — 洁净室 MIT 实现，对标 Rust crates.io #145 `num-integer`（8.88 亿+ 总下载）。

Zero-allocation integer number theory & exact floor/ceil/Euclidean division engine for CP — clean-room MIT parity with Rust `num-integer` (crates.io #145, 888M+ downloads).

## 安装 / Install

```bash
cap add @cap/num-integer
```

## API 参考 / API Reference

| 符号 / Symbol | 说明 / Description |
| --- | --- |
| `divFloor` / `modFloor` | 向负无穷取整的地板除法与同号模 |
| `divCeil` | 向正无穷取整的天花板除法 |
| `divEuclid` / `remEuclid` | 欧几里得除法与恒非负余数 `[0, \|b\|)` |
| `gcdU64` / `lcmU64` / `gcdI64` / `lcmI64` | 最大公约数与防溢出最小公倍数 |
| `ExtendedGcd` / `extendedGcd` | 扩展欧几里得算法（满足 `a*x + b*y == gcd`） |
| `modInverse(a, m)` | 模逆元（不互质返回 `null`） |
| `isqrtU64` / `icbrtU64` | 全 `u64` 值域精确整数平方根与立方根 |
| `binomialU64(n, k)` | 防中间溢出组合数 `C(n, k)` |
| `isPrimeU64(n)` | 6k±1 确定性素性判断 |
| `averageFloor(a, b)` | 无溢出地板均值 `(a & b) + ((a ^ b) >> 1)` |

## 使用示例 / Usage

```cp
import { divFloor, modFloor, extendedGcd, modInverse, isqrtU64, binomialU64, isPrimeU64 } from "@cap/num-integer";

const q = divFloor(-7, 3);              // -3
const r = modFloor(-7, 3);              // 2
const eg = extendedGcd(240, 46);        // gcd=2, x=-9, y=47
const inv = modInverse(3, 11).?;        // 4 (3 * 4 == 1 mod 11)
const sq = isqrtU64(1000000 as u64);    // 1000
const c = binomialU64(10 as u64, 3 as u64); // 120
const p = isPrimeU64(1000000007 as u64);    // true
```

## License

MIT
