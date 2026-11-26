// Type definitions for CP

export fn minF64(a: f64, b: f64): f64;
export fn maxF64(a: f64, b: f64): f64;
export class Pt2 {
    x: f64,
    y: f64,
    fn new(x: f64, y: f64): Pt2;
}

export class Aabb2 {
    min_x: f64,
    min_y: f64,
    max_x: f64,
    max_y: f64,
    fn new(min_x: f64, min_y: f64, max_x: f64, max_y: f64): Aabb2;
    fn empty(): Aabb2;
    fn fromCenterSize(cx: f64, cy: f64, w: f64, h: f64): Aabb2;
    fn fromPoints(pts: &[]Pt2): Aabb2;
    fn isValid(): bool;
    fn width(): f64;
    fn height(): f64;
    fn area(): f64;
    fn perimeter(): f64;
    fn center(): Pt2;
    fn containsPoint(x: f64, y: f64): bool;
    fn containsBox(other: Aabb2): bool;
    fn intersects(other: Aabb2): bool;
    fn intersection(other: Aabb2): Aabb2;
    fn unionBox(other: Aabb2): Aabb2;
    fn expandBy(margin: f64): Aabb2;
    fn expandToPoint(x: f64, y: f64): Aabb2;
    fn distanceSqToPoint(x: f64, y: f64): f64;
    fn rayIntersects(ox: f64, oy: f64, dx: f64, dy: f64): bool;
}

export fn new(min_x: f64, min_y: f64, max_x: f64, max_y: f64): Aabb2;
export fn fromCenterSize(cx: f64, cy: f64, w: f64, h: f64): Aabb2;
export fn fromPoints(pts: &[]Pt2): Aabb2;
