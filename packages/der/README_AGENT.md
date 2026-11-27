# der — AI Agent Quick Ref

Zero-allocation ITU-T X.690 ASN.1 DER encoder and strict decoder reusing const-oid for OID TLV handling (RustCrypto der parity) in pure CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    DerHeader,
    DerTlvResult,
    DerSliceResult,
    DerBoolResult,
    DerU64Result,
    DerBitStringResult,
    DerSequenceResult,
    DerReader,
} from "der";
```

## Core Types & Constants

```cp
class DerHeader { tag: u8, len: usize, header_len: usize, ok: bool }
class DerTlvResult { tag: u8, value: &[]u8, ok: bool }
class DerSliceResult { data: &[]u8, ok: bool }
class DerBoolResult { value: bool, ok: bool }
class DerU64Result { value: u64, ok: bool }
class DerBitStringResult { unused_bits: u8, bytes: &[]u8, ok: bool }
class DerSequenceResult { reader: DerReader, ok: bool }
class DerReader { buf: &[]u8, pos: usize } // methods: new(...) +12
class DerWriter { buf: []u8, pos: usize } // methods: new(...) +13
```

## API Signatures

```cp
fn derEmptySlice(): &[]u8;
fn derMinBytesUsize(v: usize): usize;
```

## Minimal Usage

```cp
import { derEmptySlice } from "der";

pub fn main() {
    let result = derEmptySlice();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"der"`; install via `cap add der`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
