# morton-encoding — AI Agent Quick Ref

Zero-allocation 2D/3D Z-Order (Morton) bit-interleaving codec and 2D Hilbert space-filling curve mapper for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Point2D,
    Point3D,
    splitBy1,
    compactBy1,
    encode2D,
    decode2D,
    splitBy2,
    compactBy2,
} from "morton-encoding";
```

## Core Types & Constants

```cp
class Point2D { x: u32, y: u32 }
class Point3D { x: u32, y: u32, z: u32 }
```

## API Signatures

```cp
fn splitBy1(v: u32): u64;
fn compactBy1(v: u64): u32;
fn encode2D(x: u32, y: u32): u64;
fn decode2D(code: u64): Point2D;
fn splitBy2(v: u32): u64;
fn compactBy2(v: u64): u32;
fn encode3D(x: u32, y: u32, z: u32): u64;
fn decode3D(code: u64): Point3D;
fn clampOrder(order: u32): u32;
fn hilbertRot(n: u32, x_in: u32, y_in: u32, rx: u32, ry: u32): Point2D;
fn hilbert2DEncode(order: u32, x_in: u32, y_in: u32): u64;
fn hilbert2DDecode(order: u32, d_in: u64): Point2D;
```

## Minimal Usage

```cp
import { encode2D } from "morton-encoding";

pub fn main() {
    let result = encode2D(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"morton-encoding"`; install via `cap add morton-encoding`.
