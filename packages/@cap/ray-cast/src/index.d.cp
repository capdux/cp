// Type definitions for CP

import { sqrt, fabs } from "../../libm/src/index.cp";
export class V3 {
    x: f64,
    y: f64,
    z: f64,
    fn new(x: f64, y: f64, z: f64): V3;
}

export class Ray3 {
    ox: f64,
    oy: f64,
    oz: f64,
    dx: f64,
    dy: f64,
    dz: f64,
    fn new(ox: f64, oy: f64, oz: f64, dx: f64, dy: f64, dz: f64): Ray3;
}

export class HitResult {
    hit: bool,
    t: f64,
    u: f64,
    v: f64,
}

export fn miss(): HitResult;
export fn v3Sub(a: V3, b: V3): V3;
export fn v3Dot(a: V3, b: V3): f64;
export fn v3Cross(a: V3, b: V3): V3;
export fn rayAt(ray: Ray3, t: f64): V3;
export fn rayPlane(ray: Ray3, plane_pt: V3, normal: V3): HitResult;
export fn raySphere(ray: Ray3, center: V3, radius: f64): HitResult;
export fn rayTriangle(ray: Ray3, v0: V3, v1: V3, v2: V3): HitResult;
export fn raySegment2D(ox: f64, oy: f64, dx: f64, dy: f64, ax: f64, ay: f64, bx: f64, by: f64): HitResult;
