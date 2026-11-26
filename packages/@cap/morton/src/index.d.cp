// Type definitions for CP

export class Point2D {
    x: u32,
    y: u32,
}

export class Point3D {
    x: u32,
    y: u32,
    z: u32,
}

export fn splitBy1(v: u32): u64;
export fn compactBy1(v: u64): u32;
export fn encode2D(x: u32, y: u32): u64;
export fn decode2D(code: u64): Point2D;
export fn splitBy2(v: u32): u64;
export fn compactBy2(v: u64): u32;
export fn encode3D(x: u32, y: u32, z: u32): u64;
export fn decode3D(code: u64): Point3D;
export fn clampOrder(order: u32): u32;
export fn hilbertRot(n: u32, x_in: u32, y_in: u32, rx: u32, ry: u32): Point2D;
export fn hilbert2DEncode(order: u32, x_in: u32, y_in: u32): u64;
export fn hilbert2DDecode(order: u32, d_in: u64): Point2D;
