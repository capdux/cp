pub struct Point2 {
    pub x: f64,
    pub y: f64,

    pub static fn new(x: f64, y: f64): Point2 {
        return Point2 { x: x, y: y };
    }

    pub static fn fromSlice(s: []f64): Point2 {
        return Point2 { x: s[0], y: s[1] };
    }

    pub fn toArray(): [2]f64 {
        let out: [2]f64 = [_.x, _.y];
        return out;
    }
}

pub struct Point3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,

    pub static fn new(x: f64, y: f64, z: f64): Point3 {
        return Point3 { x: x, y: y, z: z };
    }

    pub fn toArray(): [3]f64 {
        let out: [3]f64 = [_.x, _.y, _.z];
        return out;
    }
}

pub struct Vector2 {
    pub x: f64,
    pub y: f64,

    pub static fn new(x: f64, y: f64): Vector2 {
        return Vector2 { x: x, y: y };
    }

    pub fn dot(other: Vector2): f64 {
        return _.x * other.x + _.y * other.y;
    }

    pub fn lenSq(): f64 {
        return _.x * _.x + _.y * _.y;
    }

    pub fn toArray(): [2]f64 {
        let out: [2]f64 = [_.x, _.y];
        return out;
    }
}

pub struct Vector3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,

    pub static fn new(x: f64, y: f64, z: f64): Vector3 {
        return Vector3 { x: x, y: y, z: z };
    }

    pub fn dot(other: Vector3): f64 {
        return _.x * other.x + _.y * other.y + _.z * other.z;
    }

    pub fn cross(other: Vector3): Vector3 {
        return Vector3 {
            x: _.y * other.z - _.z * other.y,
            y: _.z * other.x - _.x * other.z,
            z: _.x * other.y - _.y * other.x,
        };
    }

    pub fn lenSq(): f64 {
        return _.x * _.x + _.y * _.y + _.z * _.z;
    }

    pub fn toArray(): [3]f64 {
        let out: [3]f64 = [_.x, _.y, _.z];
        return out;
    }
}

pub struct Vector4 {
    pub x: f64,
    pub y: f64,
    pub z: f64,
    pub w: f64,

    pub static fn new(x: f64, y: f64, z: f64, w: f64): Vector4 {
        return Vector4 { x: x, y: y, z: z, w: w };
    }

    pub fn dot(other: Vector4): f64 {
        return _.x * other.x + _.y * other.y + _.z * other.z + _.w * other.w;
    }

    pub fn toArray(): [4]f64 {
        let out: [4]f64 = [_.x, _.y, _.z, _.w];
        return out;
    }
}

pub struct Quaternion {
    pub v: Vector3,
    pub s: f64,

    pub static fn identity(): Quaternion {
        return Quaternion { v: Vector3.new(0.0, 0.0, 0.0), s: 1.0 };
    }

    pub static fn new(x: f64, y: f64, z: f64, s: f64): Quaternion {
        return Quaternion { v: Vector3.new(x, y, z), s: s };
    }

    pub fn conjugate(): Quaternion {
        return Quaternion {
            v: Vector3.new(-_.v.x, -_.v.y, -_.v.z),
            s: _.s,
        };
    }

    pub fn normSq(): f64 {
        return _.v.lenSq() + _.s * _.s;
    }

    pub fn mul(rhs: Quaternion): Quaternion {
        let cr = _.v.cross(rhs.v);
        let d = _.v.dot(rhs.v);
        return Quaternion {
            v: Vector3.new(
                _.s * rhs.v.x + rhs.s * _.v.x + cr.x,
                _.s * rhs.v.y + rhs.s * _.v.y + cr.y,
                _.s * rhs.v.z + rhs.s * _.v.z + cr.z
            ),
            s: _.s * rhs.s - d,
        };
    }
}

pub struct EulerAngles {
    pub a: f64,
    pub b: f64,
    pub c: f64,

    pub static fn new(a: f64, b: f64, c: f64): EulerAngles {
        return EulerAngles { a: a, b: b, c: c };
    }
}

pub struct RowMatrix2 {
    pub x: Vector2,
    pub y: Vector2,

    pub static fn identity(): RowMatrix2 {
        return RowMatrix2 {
            x: Vector2.new(1.0, 0.0),
            y: Vector2.new(0.0, 1.0),
        };
    }

    pub fn det(): f64 {
        return _.x.x * _.y.y - _.x.y * _.y.x;
    }

    pub fn transpose(): RowMatrix2 {
        return RowMatrix2 {
            x: Vector2.new(_.x.x, _.y.x),
            y: Vector2.new(_.x.y, _.y.y),
        };
    }
}

pub struct RowMatrix3 {
    pub x: Vector3,
    pub y: Vector3,
    pub z: Vector3,

    pub static fn identity(): RowMatrix3 {
        return RowMatrix3 {
            x: Vector3.new(1.0, 0.0, 0.0),
            y: Vector3.new(0.0, 1.0, 0.0),
            z: Vector3.new(0.0, 0.0, 1.0),
        };
    }

    pub fn transpose(): RowMatrix3 {
        return RowMatrix3 {
            x: Vector3.new(_.x.x, _.y.x, _.z.x),
            y: Vector3.new(_.x.y, _.y.y, _.z.y),
            z: Vector3.new(_.x.z, _.y.z, _.z.z),
        };
    }

    pub fn mulVec3(v: Vector3): Vector3 {
        return Vector3.new(_.x.dot(v), _.y.dot(v), _.z.dot(v));
    }
}

pub struct RowMatrix4 {
    pub x: Vector4,
    pub y: Vector4,
    pub z: Vector4,
    pub w: Vector4,

    pub static fn identity(): RowMatrix4 {
        return RowMatrix4 {
            x: Vector4.new(1.0, 0.0, 0.0, 0.0),
            y: Vector4.new(0.0, 1.0, 0.0, 0.0),
            z: Vector4.new(0.0, 0.0, 1.0, 0.0),
            w: Vector4.new(0.0, 0.0, 0.0, 1.0),
        };
    }

    pub fn transpose(): RowMatrix4 {
        return RowMatrix4 {
            x: Vector4.new(_.x.x, _.y.x, _.z.x, _.w.x),
            y: Vector4.new(_.x.y, _.y.y, _.z.y, _.w.y),
            z: Vector4.new(_.x.z, _.y.z, _.z.z, _.w.z),
            w: Vector4.new(_.x.w, _.y.w, _.z.w, _.w.w),
        };
    }

    pub fn mulVec4(v: Vector4): Vector4 {
        return Vector4.new(_.x.dot(v), _.y.dot(v), _.z.dot(v), _.w.dot(v));
    }

    pub fn toColumnMajor(): ColumnMatrix4 {
        let t = _.transpose();
        return ColumnMatrix4 { x: t.x, y: t.y, z: t.z, w: t.w };
    }
}

pub struct ColumnMatrix4 {
    pub x: Vector4,
    pub y: Vector4,
    pub z: Vector4,
    pub w: Vector4,

    pub static fn identity(): ColumnMatrix4 {
        return ColumnMatrix4 {
            x: Vector4.new(1.0, 0.0, 0.0, 0.0),
            y: Vector4.new(0.0, 1.0, 0.0, 0.0),
            z: Vector4.new(0.0, 0.0, 1.0, 0.0),
            w: Vector4.new(0.0, 0.0, 0.0, 1.0),
        };
    }

    pub fn toRowMajor(): RowMatrix4 {
        let r = RowMatrix4 { x: _.x, y: _.y, z: _.z, w: _.w };
        return r.transpose();
    }
}

pub fn point3ToVector3(p: Point3): Vector3 {
    return Vector3.new(p.x, p.y, p.z);
}

pub fn vector3ToPoint3(v: Vector3): Point3 {
    return Point3.new(v.x, v.y, v.z);
}
