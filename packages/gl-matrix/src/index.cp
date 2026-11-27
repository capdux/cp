// @cap/vec3 — High-performance zero-allocation 3D vector and spatial geometry library
// MIT License

pub fn sqrtF64(x: f64): f64 {
    if (x <= 0.0) {
        return 0.0;
    }
    let g: f64 = x;
    if (g < 1.0) {
        g = 1.0;
    }
    let i: usize = 0;
    while (i < 64) {
        let next: f64 = 0.5 * (g + x / g);
        let diff: f64 = next - g;
        if (diff < 0.0) {
            diff = 0.0 - diff;
        }
        g = next;
        if (diff <= 0.000000000000001 * g) {
            break;
        }
        i = i + 1;
    }
    return g;
}

fn absF64(x: f64): f64 {
    if (x < 0.0) {
        return 0.0 - x;
    }
    return x;
}

fn minF64(a: f64, b: f64): f64 {
    if (a < b) {
        return a;
    }
    return b;
}

fn maxF64(a: f64, b: f64): f64 {
    if (a > b) {
        return a;
    }
    return b;
}

pub struct Vec3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,

    pub static fn new(x: f64, y: f64, z: f64): Vec3 {
        return Vec3 { x: x, y: y, z: z };
    }

    pub static fn zero(): Vec3 {
        return Vec3 { x: 0.0, y: 0.0, z: 0.0 };
    }

    pub static fn splat(v: f64): Vec3 {
        return Vec3 { x: v, y: v, z: v };
    }

    pub static fn unitX(): Vec3 {
        return Vec3 { x: 1.0, y: 0.0, z: 0.0 };
    }

    pub static fn unitY(): Vec3 {
        return Vec3 { x: 0.0, y: 1.0, z: 0.0 };
    }

    pub static fn unitZ(): Vec3 {
        return Vec3 { x: 0.0, y: 0.0, z: 1.0 };
    }

    pub fn add(other: Vec3): Vec3 {
        return Vec3 {
            x: _.x + other.x,
            y: _.y + other.y,
            z: _.z + other.z,
        };
    }

    pub fn sub(other: Vec3): Vec3 {
        return Vec3 {
            x: _.x - other.x,
            y: _.y - other.y,
            z: _.z - other.z,
        };
    }

    pub fn mul(other: Vec3): Vec3 {
        return Vec3 {
            x: _.x * other.x,
            y: _.y * other.y,
            z: _.z * other.z,
        };
    }

    pub fn scale(s: f64): Vec3 {
        return Vec3 {
            x: _.x * s,
            y: _.y * s,
            z: _.z * s,
        };
    }

    pub fn neg(): Vec3 {
        return Vec3 {
            x: 0.0 - _.x,
            y: 0.0 - _.y,
            z: 0.0 - _.z,
        };
    }

    pub fn dot(other: Vec3): f64 {
        return _.x * other.x + _.y * other.y + _.z * other.z;
    }

    pub fn cross(other: Vec3): Vec3 {
        return Vec3 {
            x: _.y * other.z - _.z * other.y,
            y: _.z * other.x - _.x * other.z,
            z: _.x * other.y - _.y * other.x,
        };
    }

    pub fn lengthSq(): f64 {
        return _.x * _.x + _.y * _.y + _.z * _.z;
    }

    pub fn length(): f64 {
        return sqrtF64(_.lengthSq());
    }

    pub fn distance(other: Vec3): f64 {
        let dx: f64 = _.x - other.x;
        let dy: f64 = _.y - other.y;
        let dz: f64 = _.z - other.z;
        return sqrtF64(dx * dx + dy * dy + dz * dz);
    }

    pub fn normalize(): Vec3 {
        let len: f64 = _.length();
        if (len <= 0.0) {
            return Vec3.zero();
        }
        let inv: f64 = 1.0 / len;
        return Vec3 {
            x: _.x * inv,
            y: _.y * inv,
            z: _.z * inv,
        };
    }

    pub fn lerp(other: Vec3, t: f64): Vec3 {
        return Vec3 {
            x: _.x + (other.x - _.x) * t,
            y: _.y + (other.y - _.y) * t,
            z: _.z + (other.z - _.z) * t,
        };
    }

    pub fn reflect(normal: Vec3): Vec3 {
        let n: Vec3 = normal.normalize();
        if (n.lengthSq() <= 0.0) {
            return Vec3 { x: _.x, y: _.y, z: _.z };
        }
        let k: f64 = 2.0 * _.dot(n);
        return Vec3 {
            x: _.x - n.x * k,
            y: _.y - n.y * k,
            z: _.z - n.z * k,
        };
    }

    pub fn project(onto: Vec3): Vec3 {
        let denom: f64 = onto.lengthSq();
        if (denom <= 0.0) {
            return Vec3.zero();
        }
        let k: f64 = _.dot(onto) / denom;
        return Vec3 {
            x: onto.x * k,
            y: onto.y * k,
            z: onto.z * k,
        };
    }

    pub fn minVec(other: Vec3): Vec3 {
        return Vec3 {
            x: minF64(_.x, other.x),
            y: minF64(_.y, other.y),
            z: minF64(_.z, other.z),
        };
    }

    pub fn maxVec(other: Vec3): Vec3 {
        return Vec3 {
            x: maxF64(_.x, other.x),
            y: maxF64(_.y, other.y),
            z: maxF64(_.z, other.z),
        };
    }

    pub fn clampLength(max_len: f64): Vec3 {
        if (max_len <= 0.0) {
            return Vec3.zero();
        }
        let len_sq: f64 = _.lengthSq();
        let max_sq: f64 = max_len * max_len;
        if (len_sq <= max_sq || len_sq <= 0.0) {
            return Vec3 { x: _.x, y: _.y, z: _.z };
        }
        let k: f64 = max_len / sqrtF64(len_sq);
        return Vec3 {
            x: _.x * k,
            y: _.y * k,
            z: _.z * k,
        };
    }

    pub fn approxEq(other: Vec3, eps: f64): bool {
        return absF64(_.x - other.x) <= eps &&
            absF64(_.y - other.y) <= eps &&
            absF64(_.z - other.z) <= eps;
    }
}

pub fn new(x: f64, y: f64, z: f64): Vec3 {
    return Vec3.new(x, y, z);
}

pub fn zero(): Vec3 {
    return Vec3.zero();
}

pub fn splat(v: f64): Vec3 {
    return Vec3.splat(v);
}

pub fn unitX(): Vec3 {
    return Vec3.unitX();
}

pub fn unitY(): Vec3 {
    return Vec3.unitY();
}

pub fn unitZ(): Vec3 {
    return Vec3.unitZ();
}

pub fn add(a: Vec3, b: Vec3): Vec3 {
    return a.add(b);
}

pub fn sub(a: Vec3, b: Vec3): Vec3 {
    return a.sub(b);
}

pub fn mul(a: Vec3, b: Vec3): Vec3 {
    return a.mul(b);
}

pub fn scale(v: Vec3, s: f64): Vec3 {
    return v.scale(s);
}

pub fn neg(v: Vec3): Vec3 {
    return v.neg();
}

pub fn dot(a: Vec3, b: Vec3): f64 {
    return a.dot(b);
}

pub fn cross(a: Vec3, b: Vec3): Vec3 {
    return a.cross(b);
}

pub fn lengthSq(v: Vec3): f64 {
    return v.lengthSq();
}

pub fn length(v: Vec3): f64 {
    return v.length();
}

pub fn distance(a: Vec3, b: Vec3): f64 {
    return a.distance(b);
}

pub fn normalize(v: Vec3): Vec3 {
    return v.normalize();
}

pub fn lerp(a: Vec3, b: Vec3, t: f64): Vec3 {
    return a.lerp(b, t);
}

pub fn reflect(v: Vec3, normal: Vec3): Vec3 {
    return v.reflect(normal);
}

pub fn project(v: Vec3, onto: Vec3): Vec3 {
    return v.project(onto);
}

pub fn minVec(a: Vec3, b: Vec3): Vec3 {
    return a.minVec(b);
}

pub fn maxVec(a: Vec3, b: Vec3): Vec3 {
    return a.maxVec(b);
}

pub fn clampLength(v: Vec3, max_len: f64): Vec3 {
    return v.clampLength(max_len);
}

pub fn approxEq(a: Vec3, b: Vec3, eps: f64): bool {
    return a.approxEq(b, eps);
}
