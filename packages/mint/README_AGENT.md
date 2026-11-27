# mint — AI Agent Quick Ref

Zero-heap Math Interoperability Standard Types: Point2/3, Vector2/3/4, Quaternion, RowMatrix2/3/4, ColumnMatrix4, EulerAngles (crates.io mint equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Point2,
    Point3,
    Vector2,
    Vector3,
    Vector4,
    Quaternion,
    EulerAngles,
    RowMatrix2,
} from "mint";
```

## Core Types & Constants

```cp
class Point2 { x: f64, y: f64 } // methods: new(...) +2
class Point3 { x: f64, y: f64, z: f64 } // methods: new(...) +1
class Vector2 { x: f64, y: f64 } // methods: new(...) +3
class Vector3 { x: f64, y: f64, z: f64 } // methods: new(...) +4
class Vector4 { x: f64, y: f64, z: f64, w: f64 } // methods: new(...) +2
class Quaternion { v: Vector3, s: f64 } // methods: identity(...) +4
class EulerAngles { a: f64, b: f64, c: f64 } // methods: new(...)
class RowMatrix2 { x: Vector2, y: Vector2 } // methods: identity(...) +2
class RowMatrix3 { x: Vector3, y: Vector3, z: Vector3 } // methods: identity(...) +2
class RowMatrix4 { x: Vector4, y: Vector4, z: Vector4, w: Vector4 } // methods: identity(...) +3
class ColumnMatrix4 { x: Vector4, y: Vector4, z: Vector4, w: Vector4 } // methods: identity(...) +1
```

## API Signatures

```cp
fn point3ToVector3(p: Point3): Vector3;
fn vector3ToPoint3(v: Vector3): Point3;
```

## Minimal Usage

```cp
import { point3ToVector3 } from "mint";

pub fn main() {
    let result = point3ToVector3(/* p: Point3 */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"mint"`; install via `cap add mint`.
