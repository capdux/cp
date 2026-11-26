// Type definitions for CP

export class Pt3 {
    x: f64,
    y: f64,
    z: f64,
}

export fn abAbs(x: f64): f64;
export fn abMin(a: f64, b: f64): f64;
export fn abMax(a: f64, b: f64): f64;
export class Aabb3 {
    min_x: f64,
    min_y: f64,
    min_z: f64,
    max_x: f64,
    max_y: f64,
    max_z: f64,
    fn new(min_x: f64, min_y: f64, min_z: f64, max_x: f64, max_y: f64, max_z: f64): Aabb3;
    fn fromCenterSize(cx: f64, cy: f64, cz: f64, sx: f64, sy: f64, sz: f64): Aabb3;
    fn isValid(): bool;
    fn volume(): f64;
    fn surfaceArea(): f64;
    fn center(): Pt3;
    fn containsPoint(x: f64, y: f64, z: f64): bool;
    fn intersects(other: Aabb3): bool;
    fn intersection(other: Aabb3): Aabb3;
    fn unionBox(other: Aabb3): Aabb3;
    fn expandBy(margin: f64): Aabb3;
    fn distanceSqToPoint(x: f64, y: f64, z: f64): f64;
    fn rayIntersects(ox: f64, oy: f64, oz: f64, dx: f64, dy: f64, dz: f64): bool;
}

