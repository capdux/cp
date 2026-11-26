// @cap/ray-cast — Zero-allocation 2D/3D geometric ray intersection library
// Reuses @cap/libm for sqrt & fabs (MIT License)

import { sqrt, fabs } from "../../libm/src/index.cp";

const EPS: f64 = 0.000000000001;

pub struct V3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,

    pub static fn new(x: f64, y: f64, z: f64): V3 {
        return V3 { x: x, y: y, z: z };
    }
}

pub struct Ray3 {
    pub ox: f64,
    pub oy: f64,
    pub oz: f64,
    pub dx: f64,
    pub dy: f64,
    pub dz: f64,

    pub static fn new(ox: f64, oy: f64, oz: f64, dx: f64, dy: f64, dz: f64): Ray3 {
        return Ray3 { ox: ox, oy: oy, oz: oz, dx: dx, dy: dy, dz: dz };
    }
}

pub struct HitResult {
    pub hit: bool,
    pub t: f64,
    pub u: f64,
    pub v: f64,
}

fn miss(): HitResult {
    return HitResult { hit: false, t: 0.0, u: 0.0, v: 0.0 };
}

fn v3Sub(a: V3, b: V3): V3 {
    return V3 { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z };
}

fn v3Dot(a: V3, b: V3): f64 {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

fn v3Cross(a: V3, b: V3): V3 {
    return V3 {
        x: a.y * b.z - a.z * b.y,
        y: a.z * b.x - a.x * b.z,
        z: a.x * b.y - a.y * b.x,
    };
}

pub fn rayAt(ray: Ray3, t: f64): V3 {
    return V3 {
        x: ray.ox + ray.dx * t,
        y: ray.oy + ray.dy * t,
        z: ray.oz + ray.dz * t,
    };
}

pub fn rayPlane(ray: Ray3, plane_pt: V3, normal: V3): HitResult {
    let dir: V3 = V3 { x: ray.dx, y: ray.dy, z: ray.dz };
    let denom: f64 = v3Dot(dir, normal);
    if (fabs(denom) <= EPS) {
        return miss();
    }
    let origin: V3 = V3 { x: ray.ox, y: ray.oy, z: ray.oz };
    let t: f64 = v3Dot(v3Sub(plane_pt, origin), normal) / denom;
    if (t < 0.0) {
        return miss();
    }
    return HitResult { hit: true, t: t, u: 0.0, v: 0.0 };
}

pub fn raySphere(ray: Ray3, center: V3, radius: f64): HitResult {
    if (radius < 0.0) {
        return miss();
    }
    let dir: V3 = V3 { x: ray.dx, y: ray.dy, z: ray.dz };
    let a: f64 = v3Dot(dir, dir);
    if (a <= EPS) {
        return miss();
    }
    let origin: V3 = V3 { x: ray.ox, y: ray.oy, z: ray.oz };
    let oc: V3 = v3Sub(origin, center);
    let b: f64 = 2.0 * v3Dot(oc, dir);
    let c: f64 = v3Dot(oc, oc) - radius * radius;
    let disc: f64 = b * b - 4.0 * a * c;
    if (disc < 0.0) {
        return miss();
    }
    let s: f64 = sqrt(disc);
    let inv2a: f64 = 1.0 / (2.0 * a);
    let t0: f64 = (0.0 - b - s) * inv2a;
    if (t0 >= 0.0) {
        return HitResult { hit: true, t: t0, u: 0.0, v: 0.0 };
    }
    let t1: f64 = (0.0 - b + s) * inv2a;
    if (t1 >= 0.0) {
        return HitResult { hit: true, t: t1, u: 0.0, v: 0.0 };
    }
    return miss();
}

pub fn rayTriangle(ray: Ray3, v0: V3, v1: V3, v2: V3): HitResult {
    let dir: V3 = V3 { x: ray.dx, y: ray.dy, z: ray.dz };
    let e1: V3 = v3Sub(v1, v0);
    let e2: V3 = v3Sub(v2, v0);
    let h: V3 = v3Cross(dir, e2);
    let det: f64 = v3Dot(e1, h);
    if (fabs(det) <= EPS) {
        return miss();
    }
    let inv_det: f64 = 1.0 / det;
    let origin: V3 = V3 { x: ray.ox, y: ray.oy, z: ray.oz };
    let s: V3 = v3Sub(origin, v0);
    let u: f64 = inv_det * v3Dot(s, h);
    if (u < 0.0 || u > 1.0) {
        return miss();
    }
    let q: V3 = v3Cross(s, e1);
    let v: f64 = inv_det * v3Dot(dir, q);
    if (v < 0.0 || u + v > 1.0) {
        return miss();
    }
    let t: f64 = inv_det * v3Dot(e2, q);
    if (t < 0.0) {
        return miss();
    }
    return HitResult { hit: true, t: t, u: u, v: v };
}

pub fn raySegment2D(
    ox: f64,
    oy: f64,
    dx: f64,
    dy: f64,
    ax: f64,
    ay: f64,
    bx: f64,
    by: f64
): HitResult {
    let sx: f64 = bx - ax;
    let sy: f64 = by - ay;
    let denom: f64 = dx * sy - dy * sx;
    if (fabs(denom) <= EPS) {
        return miss();
    }
    let qpx: f64 = ax - ox;
    let qpy: f64 = ay - oy;
    let t: f64 = (qpx * sy - qpy * sx) / denom;
    let u: f64 = (qpx * dy - qpy * dx) / denom;
    if (t >= 0.0 && u >= 0.0 && u <= 1.0) {
        return HitResult { hit: true, t: t, u: u, v: 0.0 };
    }
    return miss();
}
