// @cap/aabb2 — Zero-allocation 2D Axis-Aligned Bounding Box (AABB) geometry library
// MIT License

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

pub struct Pt2 {
    pub x: f64,
    pub y: f64,

    pub static fn new(x: f64, y: f64): Pt2 {
        return Pt2 { x: x, y: y };
    }
}

pub struct Aabb2 {
    pub min_x: f64,
    pub min_y: f64,
    pub max_x: f64,
    pub max_y: f64,

    pub static fn new(min_x: f64, min_y: f64, max_x: f64, max_y: f64): Aabb2 {
        return Aabb2 {
            min_x: min_x,
            min_y: min_y,
            max_x: max_x,
            max_y: max_y,
        };
    }

    pub static fn empty(): Aabb2 {
        return Aabb2 {
            min_x: 1.0,
            min_y: 1.0,
            max_x: -1.0,
            max_y: -1.0,
        };
    }

    pub static fn fromCenterSize(cx: f64, cy: f64, w: f64, h: f64): Aabb2 {
        let hw: f64 = w * 0.5;
        let hh: f64 = h * 0.5;
        return Aabb2 {
            min_x: cx - hw,
            min_y: cy - hh,
            max_x: cx + hw,
            max_y: cy + hh,
        };
    }

    pub static fn fromPoints(pts: []const Pt2): Aabb2 {
        if (pts.len == 0) {
            return Aabb2.empty();
        }
        let lx: f64 = pts[0].x;
        let ly: f64 = pts[0].y;
        let hx: f64 = pts[0].x;
        let hy: f64 = pts[0].y;
        let i: usize = 1;
        while (i < pts.len) {
            let p: Pt2 = pts[i];
            if (p.x < lx) { lx = p.x; }
            if (p.y < ly) { ly = p.y; }
            if (p.x > hx) { hx = p.x; }
            if (p.y > hy) { hy = p.y; }
            i = i + 1;
        }
        return Aabb2 { min_x: lx, min_y: ly, max_x: hx, max_y: hy };
    }

    pub fn isValid(): bool {
        return _.min_x <= _.max_x && _.min_y <= _.max_y;
    }

    pub fn width(): f64 {
        if (!_.isValid()) {
            return 0.0;
        }
        return _.max_x - _.min_x;
    }

    pub fn height(): f64 {
        if (!_.isValid()) {
            return 0.0;
        }
        return _.max_y - _.min_y;
    }

    pub fn area(): f64 {
        return _.width() * _.height();
    }

    pub fn perimeter(): f64 {
        if (!_.isValid()) {
            return 0.0;
        }
        return 2.0 * (_.width() + _.height());
    }

    pub fn center(): Pt2 {
        return Pt2 {
            x: 0.5 * (_.min_x + _.max_x),
            y: 0.5 * (_.min_y + _.max_y),
        };
    }

    pub fn containsPoint(x: f64, y: f64): bool {
        return _.isValid() &&
            x >= _.min_x && x <= _.max_x &&
            y >= _.min_y && y <= _.max_y;
    }

    pub fn containsBox(other: Aabb2): bool {
        return _.isValid() && other.isValid() &&
            other.min_x >= _.min_x && other.max_x <= _.max_x &&
            other.min_y >= _.min_y && other.max_y <= _.max_y;
    }

    pub fn intersects(other: Aabb2): bool {
        return _.isValid() && other.isValid() &&
            _.min_x <= other.max_x && _.max_x >= other.min_x &&
            _.min_y <= other.max_y && _.max_y >= other.min_y;
    }

    pub fn intersection(other: Aabb2): Aabb2 {
        if (!_.intersects(other)) {
            return Aabb2.empty();
        }
        return Aabb2 {
            min_x: maxF64(_.min_x, other.min_x),
            min_y: maxF64(_.min_y, other.min_y),
            max_x: minF64(_.max_x, other.max_x),
            max_y: minF64(_.max_y, other.max_y),
        };
    }

    pub fn unionBox(other: Aabb2): Aabb2 {
        if (!_.isValid()) {
            return other;
        }
        if (!other.isValid()) {
            return Aabb2 {
                min_x: _.min_x,
                min_y: _.min_y,
                max_x: _.max_x,
                max_y: _.max_y,
            };
        }
        return Aabb2 {
            min_x: minF64(_.min_x, other.min_x),
            min_y: minF64(_.min_y, other.min_y),
            max_x: maxF64(_.max_x, other.max_x),
            max_y: maxF64(_.max_y, other.max_y),
        };
    }

    pub fn expandBy(margin: f64): Aabb2 {
        return Aabb2 {
            min_x: _.min_x - margin,
            min_y: _.min_y - margin,
            max_x: _.max_x + margin,
            max_y: _.max_y + margin,
        };
    }

    pub fn expandToPoint(x: f64, y: f64): Aabb2 {
        if (!_.isValid()) {
            return Aabb2 { min_x: x, min_y: y, max_x: x, max_y: y };
        }
        return Aabb2 {
            min_x: minF64(_.min_x, x),
            min_y: minF64(_.min_y, y),
            max_x: maxF64(_.max_x, x),
            max_y: maxF64(_.max_y, y),
        };
    }

    pub fn distanceSqToPoint(x: f64, y: f64): f64 {
        let dx: f64 = 0.0;
        if (x < _.min_x) {
            dx = _.min_x - x;
        } else if (x > _.max_x) {
            dx = x - _.max_x;
        }
        let dy: f64 = 0.0;
        if (y < _.min_y) {
            dy = _.min_y - y;
        } else if (y > _.max_y) {
            dy = y - _.max_y;
        }
        return dx * dx + dy * dy;
    }

    pub fn rayIntersects(ox: f64, oy: f64, dx: f64, dy: f64): bool {
        if (!_.isValid()) {
            return false;
        }
        let tmin: f64 = 0.0;
        let tmax: f64 = 0.0;
        let has_max: bool = false;

        if (dx == 0.0) {
            if (ox < _.min_x || ox > _.max_x) {
                return false;
            }
        } else {
            let inv_dx: f64 = 1.0 / dx;
            let t1: f64 = (_.min_x - ox) * inv_dx;
            let t2: f64 = (_.max_x - ox) * inv_dx;
            let lo: f64 = minF64(t1, t2);
            let hi: f64 = maxF64(t1, t2);
            if (lo > tmin) {
                tmin = lo;
            }
            tmax = hi;
            has_max = true;
        }

        if (dy == 0.0) {
            if (oy < _.min_y || oy > _.max_y) {
                return false;
            }
        } else {
            let inv_dy: f64 = 1.0 / dy;
            let t1: f64 = (_.min_y - oy) * inv_dy;
            let t2: f64 = (_.max_y - oy) * inv_dy;
            let lo: f64 = minF64(t1, t2);
            let hi: f64 = maxF64(t1, t2);
            if (lo > tmin) {
                tmin = lo;
            }
            if (!has_max || hi < tmax) {
                tmax = hi;
                has_max = true;
            }
        }

        if (!has_max) {
            return true;
        }
        return tmax >= tmin && tmax >= 0.0;
    }
}

pub fn new(min_x: f64, min_y: f64, max_x: f64, max_y: f64): Aabb2 {
    return Aabb2.new(min_x, min_y, max_x, max_y);
}

pub fn fromCenterSize(cx: f64, cy: f64, w: f64, h: f64): Aabb2 {
    return Aabb2.fromCenterSize(cx, cy, w, h);
}

pub fn fromPoints(pts: []const Pt2): Aabb2 {
    return Aabb2.fromPoints(pts);
}
