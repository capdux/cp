# bn.js@1.0.0 (Agent Quickref)

> Zero-allocation 256-bit unsigned big integer arithmetic (4x u64 limbs) for CP | License: MIT
> Import: `import { ... } from "bn.js";` | Full types: `src/index.d.cp`

## Functions
- `export fn addWithCarry64(a: u64, b: u64, carry_in: u64): Add64Res;`
- `export fn subWithBorrow64(a: u64, b: u64, borrow_in: u64): Sub64Res;`
- `export fn mulWide64(a: u64, b: u64): Mul64Res;`
- `export fn clz64(x: u64): u32;`
- `export fn hexVal(c: u8): i32;`
- `export fn hexDigit(v: u8): u8;`
- `export fn zero(): U256;`
- `export fn one(): U256;`
- `export fn fromU64(v: u64): U256;`
- `export fn fromU128(lo: u64, hi: u64): U256;`
- `export fn maxVal(): U256;`
- `export fn fromHex(s: string): U256;`
