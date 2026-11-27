# crypto-bigint — AI Agent Quick Ref

Zero-allocation constant-time fixed-width 256-bit unsigned integer (U256) arithmetic and modular operations (RustCrypto crypto-bigint parity) for CP (`v1.0.0`, `Apache-2.0 OR MIT`)

## Import

```cp
import {
    CbiAddRes,
    CbiSubRes,
    U256,
    cbiMask64,
    cbiAllOnes64,
    cbiCtMask,
    cbiReadBeU64,
    cbiReadLeU64,
} from "crypto-bigint";
```

## Core Types & Constants

```cp
class CbiAddRes { val: U256, carry: u64 }
class CbiSubRes { val: U256, borrow: u64 }
class U256 { limbs: [4]u64 } // methods: fromU64(...) +17
```

## API Signatures

```cp
fn cbiMask64(): u128;
fn cbiAllOnes64(): u64;
fn cbiCtMask(cond: bool): u64;
fn cbiReadBeU64(b: &[]u8, off: usize): u64;
fn cbiReadLeU64(b: &[]u8, off: usize): u64;
fn cbiWriteBeU64(v: u64, out: []u8, off: usize);
fn cbiWriteLeU64(v: u64, out: []u8, off: usize);
fn cbiHexNibble(ch: u8): i32;
fn cbiHexChar(nibble: u8): u8;
fn cbiBitLen64(v_in: u64): usize;
fn cbiAddWithCarry(a: U256, b: U256): CbiAddRes;
fn cbiSubWithBorrow(a: U256, b: U256): CbiSubRes;
fn addMod(a: U256, b: U256, m: U256): U256;
fn subMod(a: U256, b: U256, m: U256): U256;
```

## Minimal Usage

```cp
import { cbiMask64 } from "crypto-bigint";

pub fn main() {
    let result = cbiMask64();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"crypto-bigint"`; install via `cap add crypto-bigint`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
