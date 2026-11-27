// packages/@cap/aabb3/src/index.cp
// Zero-allocation 3D Axis-Aligned Bounding Box (AABB) & Kay-Kajiya Slab raycast engine.
// MIT clean-room pure CP implementation.

/// 3D point in Cartesian space.
pub struct Pt3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

fn abAbs(x: f64): f64 {
    if (x < (0.0 as f64)) { return -x; }
    return x;
}

fn abMin(a: f64, b: f64): f64 {
    if (a < b) { return a; }
    return b;
}

fn abMax(a: f64, b: f64): f64 {
    if (a > b) { return a; }
    return b;
}

/// 3D Axis-Aligned Bounding Box defined by minimum and maximum corner coordinates.
pub struct Aabb3 {
    pub min_x: f64,
    pub min_y: f64,
    pub min_z: f64,
    pub max_x: f64,
    pub max_y: f64,
    pub max_z: f64,

    /// Construct an `Aabb3` from explicit min and max corner coordinates.
    pub static fn new(min_x: f64, min_y: f64, min_z: f64, max_x: f64, max_y: f64, max_z: f64): Aabb3 {
        return Aabb3{
            min_x: min_x,
            min_y: min_y,
            min_z: min_z,
            max_x: max_x,
            max_y: max_y,
            max_z: max_z,
        };
    }

    /// Construct an `Aabb3` centered at `(cx, cy, cz)` with total extents `(sx, sy, sz)`.
    pub static fn fromCenterSize(cx: f64, cy: f64, cz: f64, sx: f64, sy: f64, sz: f64): Aabb3 {
        const hx: f64 = sx * (0.5 as f64);
        const hy: f64 = sy * (0.5 as f64);
        const hz: f64 = sz * (0.5 as f64);
        return Aabb3{
            min_x: cx - hx,
            min_y: cy - hy,
            min_z: cz - hz,
            max_x: cx + hx,
            max_y: cy + hy,
            max_z: cz + hz,
        };
    }

    /// Returns true if `min <= max` along all three axes.
    pub fn isValid(): bool {
        return _.min_x <= _.max_x && _.min_y <= _.max_y && _.min_z <= _.max_z;
    }

    /// Compute the enclosed 3D volume (returns `0.0` if invalid).
    pub fn volume(): f64 {
        if (!_.isValid()) {
            return 0.0 as f64;
        }
        return (_.max_x - _.min_x) * (_.max_y - _.min_y) * (_.max_z - _.min_z);
    }

    /// Compute the total 3D surface area (returns `0.0` if invalid).
    pub fn surfaceArea(): f64 {
        if (!_.isValid()) {
            return 0.0 as f64;
        }
        const dx: f64 = _.max_x - _.min_x;
        const dy: f64 = _.max_y - _.min_y;
        const dz: f64 = _.max_z - _.min_z;
        return (2.0 as f64) * ((dx * dy) + (dy * dz) + (dz * dx));
    }

    /// Return the geometric center point of the box.
    pub fn center(): Pt3 {
        return Pt3{
            x: (_.min_x + _.max_x) * (0.5 as f64),
            y: (_.min_y + _.max_y) * (0.5 as f64),
            z: (_.min_z + _.max_z) * (0.5 as f64),
        };
    }

    /// Returns true if point `(x, y, z)` lies inside or on the boundary of the box.
    pub fn containsPoint(x: f64, y: f64, z: f64): bool {
        if (!_.isValid()) {
            return false;
        }
        return x >= _.min_x && x <= _.max_x && y >= _.min_y && y <= _.max_y && z >= _.min_z && z <= _.max_z;
    }

    /// Returns true if this box overlaps `other` (including touching faces/edges).
    pub fn intersects(other: Aabb3): bool {
        if (!_.isValid() || !other.isValid()) {
            return false;
        }
        if (_.max_x < other.min_x || _.min_x > other.max_x) { return false; }
        if (_.max_y < other.min_y || _.min_y > other.max_y) { return false; }
        if (_.max_z < other.min_z || _.min_z > other.max_z) { return false; }
        return true;
    }

    /// Return the overlapping intersection box (`isValid() == false` if disjoint).
    pub fn intersection(other: Aabb3): Aabb3 {
        return Aabb3{
            min_x: abMax(_.min_x, other.min_x),
            min_y: abMax(_.min_y, other.min_y),
            min_z: abMax(_.min_z, other.min_z),
            max_x: abMin(_.max_x, other.max_x),
            max_y: abMin(_.max_y, other.max_y),
            max_z: abMin(_.max_z, other.max_z),
        };
    }

    /// Return the smallest bounding box enclosing both this box and `other`.
    pub fn unionBox(other: Aabb3): Aabb3 {
        return Aabb3{
            min_x: abMin(_.min_x, other.min_x),
            min_y: abMin(_.min_y, other.min_y),
            min_z: abMin(_.min_z, other.min_z),
            max_x: abMax(_.max_x, other.max_x),
            max_y: abMax(_.max_y, other.max_y),
            max_z: abMax(_.max_z, other.max_z),
        };
    }

    /// Uniformly expand (or shrink if negative) all 6 faces by `margin`.
    pub fn expandBy(margin: f64): Aabb3 {
        return Aabb3{
            min_x: _.min_x - margin,
            min_y: _.min_y - margin,
            min_z: _.min_z - margin,
            max_x: _.max_x + margin,
            max_y: _.max_y + margin,
            max_z: _.max_z + margin,
        };
    }

    /// Compute the squared Euclidean distance from point `(x, y, z)` to the box (`0.0` if inside).
    pub fn distanceSqToPoint(x: f64, y: f64, z: f64): f64 {
        let dx: f64 = 0.0;
        if (x < _.min_x) { dx = _.min_x - x; } else if (x > _.max_x) { dx = x - _.max_x; }

        let dy: f64 = 0.0;
        if (y < _.min_y) { dy = _.min_y - y; } else if (y > _.max_y) { dy = y - _.max_y; }

        let dz: f64 = 0.0;
        if (z < _.min_z) { dz = _.min_z - z; } else if (z > _.max_z) { dz = z - _.max_z; }

        return (dx * dx) + (dy * dy) + (dz * dz);
    }

    /// 3D Kay-Kajiya Slab ray intersection test for ray `P(t) = (ox, oy, oz) + t * (dx, dy, dz)` (`t >= 0`).
    /// Safely handles rays parallel to one or more coordinate axes.
    pub fn rayIntersects(ox: f64, oy: f64, oz: f64, dx: f64, dy: f64, dz: f64): bool {
        if (!_.isValid()) {
            return false;
        }

        const eps: f64 = 0.000000000000001;
        let tmin: f64 = 0.0;
        let tmax: f64 = 1000000000000000000.0;

        // X slab
        if (abAbs(dx) <= eps) {
            if (ox < _.min_x || ox > _.max_x) { return false; }
        } else {
            let t1: f64 = (_.min_x - ox) / dx;
            let t2: f64 = (_.max_x - ox) / dx;
            if (t1 > t2) {
                const tmp: f64 = t1;
                t1 = t2;
                t2 = tmp;
            }
            tmin = abMax(tmin, t1);
            tmax = abMin(tmax, t2);
            if (tmin > tmax) { return false; }
        }

        // Y slab
        if (abAbs(dy) <= eps) {
            if (oy < _.min_y || oy > _.max_y) { return false; }
        } else {
            let t1: f64 = (_.min_y - oy) / dy;
            let t2: f64 = (_.max_y - oy) / dy;
            if (t1 > t2) {
                const tmp: f64 = t1;
                t1 = t2;
                t2 = tmp;
            }
            tmin = abMax(tmin, t1);
            tmax = abMin(tmax, t2);
            if (tmin > tmax) { return false; }
        }

        // Z slab
        if (abAbs(dz) <= eps) {
            if (oz < _.min_z || oz > _.max_z) { return false; }
        } else {
            let t1: f64 = (_.min_z - oz) / dz;
            let t2: f64 = (_.max_z - oz) / dz;
            if (t1 > t2) {
                const tmp: f64 = t1;
                t1 = t2;
                t2 = tmp;
            }
            tmin = abMax(tmin, t1);
            tmax = abMin(tmax, t2);
            if (tmin > tmax) { return false; }
        }

        return tmax >= tmin && tmax >= (0.0 as f64);
    }
}

export {
    Pt3,
    Aabb3,
};
