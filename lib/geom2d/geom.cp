// Source: cp/lib/geom2d/geom.cp (2D Computational Geometry, Convex Hull & Collision Engine)
// Pure CP, zero-C-dependency 2D planar computational geometry primitives, intersection, polygon, convex hull, and collision engine.

const std = @import("std");

const GEOM_EPS: f64 = 0.000000000001;

/// Error set for 2D computational geometry operations.
pub err GeomError {
    InsufficientPoints,
    DegenerateGeometry,
    OutOfMemory,
}

/// Orientation of an ordered triplet of 2D points `(p, q, r)`.
pub enum Orientation {
    Collinear = 0,
    Clockwise = 1,
    CounterClockwise = 2,
}

/// 2D Euclidean point / vector with double-precision coordinates.
pub struct Point2 {
    x: f64;
    y: f64;

    /// Constructs a `Point2` from `(x, y)` coordinates.
    static fn init(x: f64, y: f64): Point2 {
        return Point2 { x: x, y: y };
    }

    /// Returns the origin `(0.0, 0.0)`.
    static fn zero(): Point2 {
        return Point2 { x: 0.0, y: 0.0 };
    }

    /// Vector addition `this + other`.
    fn add(other: Point2): Point2 {
        return Point2 { x: _.x + other.x, y: _.y + other.y };
    }

    /// Vector subtraction `this - other`.
    fn sub(other: Point2): Point2 {
        return Point2 { x: _.x - other.x, y: _.y - other.y };
    }

    /// Scalar multiplication `factor * this`.
    fn scale(factor: f64): Point2 {
        return Point2 { x: _.x * factor, y: _.y * factor };
    }

    /// 2D dot product `x1*x2 + y1*y2`.
    fn dot(other: Point2): f64 {
        return _.x * other.x + _.y * other.y;
    }

    /// 2D scalar cross product (perp-dot) `x1*y2 - y1*x2`.
    fn cross(other: Point2): f64 {
        return _.x * other.y - _.y * other.x;
    }

    /// Squared Euclidean distance to `other`.
    fn distSq(other: Point2): f64 {
        const dx = _.x - other.x;
        const dy = _.y - other.y;
        return dx * dx + dy * dy;
    }

    /// Euclidean distance to `other`.
    fn dist(other: Point2): f64 {
        return @sqrt(_.distSq(other));
    }

    /// Squared Euclidean norm `x^2 + y^2`.
    fn normSq(): f64 {
        return _.x * _.x + _.y * _.y;
    }

    /// Euclidean norm `sqrt(x^2 + y^2)`.
    fn norm(): f64 {
        return @sqrt(_.normSq());
    }

    /// Rotates the point counter-clockwise around the origin by `angle_rad` radians.
    fn rotate(angle_rad: f64): Point2 {
        const c = @cos(angle_rad);
        const s = @sin(angle_rad);
        return Point2 {
            x: _.x * c - _.y * s,
            y: _.x * s + _.y * c,
        };
    }

    /// Returns true if both coordinates are within `eps` of `other`.
    fn equals(other: Point2, eps: f64): bool {
        return @abs(_.x - other.x) <= eps && @abs(_.y - other.y) <= eps;
    }
}

/// 2D directed line segment from `a` to `b`.
pub struct Segment2 {
    a: Point2;
    b: Point2;

    /// Constructs a `Segment2` from endpoints `a` and `b`.
    static fn init(a: Point2, b: Point2): Segment2 {
        return Segment2 { a: a, b: b };
    }

    /// Squared length of the segment.
    fn lengthSq(): f64 {
        return _.a.distSq(_.b);
    }

    /// Euclidean length of the segment.
    fn length(): f64 {
        return _.a.dist(_.b);
    }

    /// Midpoint of the segment.
    fn midpoint(): Point2 {
        return Point2 {
            x: (_.a.x + _.b.x) * 0.5,
            y: (_.a.y + _.b.y) * 0.5,
        };
    }
}

/// 2D axis-aligned bounding box.
pub struct Aabb2 {
    min_x: f64;
    min_y: f64;
    max_x: f64;
    max_y: f64;

    /// Constructs a normalized `Aabb2` ensuring `min <= max` along both axes.
    static fn init(x1: f64, y1: f64, x2: f64, y2: f64): Aabb2 {
        const lo_x = if (x1 < x2) x1 else x2;
        const hi_x = if (x1 < x2) x2 else x1;
        const lo_y = if (y1 < y2) y1 else y2;
        const hi_y = if (y1 < y2) y2 else y1;
        return Aabb2 {
            min_x: lo_x,
            min_y: lo_y,
            max_x: hi_x,
            max_y: hi_y,
        };
    }

    /// Computes the tight bounding box enclosing a non-empty slice of points.
    static fn fromPoints(pts: []const Point2): Aabb2 ! GeomError {
        if (pts.len == 0) return GeomError.InsufficientPoints;
        let lo_x = pts[0].x;
        let lo_y = pts[0].y;
        let hi_x = pts[0].x;
        let hi_y = pts[0].y;
        for (i of 1..pts.len) {
            const p = pts[i];
            if (p.x < lo_x) lo_x = p.x;
            if (p.y < lo_y) lo_y = p.y;
            if (p.x > hi_x) hi_x = p.x;
            if (p.y > hi_y) hi_y = p.y;
        }
        return Aabb2 {
            min_x: lo_x,
            min_y: lo_y,
            max_x: hi_x,
            max_y: hi_y,
        };
    }

    /// Width of the bounding box.
    fn width(): f64 {
        return _.max_x - _.min_x;
    }

    /// Height of the bounding box.
    fn height(): f64 {
        return _.max_y - _.min_y;
    }

    /// Area of the bounding box.
    fn area(): f64 {
        return _.width() * _.height();
    }

    /// Geometric center of the bounding box.
    fn center(): Point2 {
        return Point2 {
            x: (_.min_x + _.max_x) * 0.5,
            y: (_.min_y + _.max_y) * 0.5,
        };
    }

    /// Returns true if point `p` lies inside or on the boundary of the box.
    fn contains(p: Point2): bool {
        return p.x >= _.min_x - GEOM_EPS &&
            p.x <= _.max_x + GEOM_EPS &&
            p.y >= _.min_y - GEOM_EPS &&
            p.y <= _.max_y + GEOM_EPS;
    }

    /// Returns true if this box overlaps or touches `other`.
    fn intersects(other: Aabb2): bool {
        return _.min_x <= other.max_x + GEOM_EPS &&
            _.max_x >= other.min_x - GEOM_EPS &&
            _.min_y <= other.max_y + GEOM_EPS &&
            _.max_y >= other.min_y - GEOM_EPS;
    }
}

/// 2D circle defined by center and radius.
pub struct Circle2 {
    center: Point2;
    radius: f64;

    /// Constructs a `Circle2` with non-negative radius clamped at 0.0.
    static fn init(center: Point2, radius: f64): Circle2 {
        const r = if (radius < 0.0) 0.0 else radius;
        return Circle2 { center: center, radius: r };
    }

    /// Area `pi * r^2`.
    fn area(): f64 {
        return std.math.pi * _.radius * _.radius;
    }

    /// Circumference `2 * pi * r`.
    fn circumference(): f64 {
        return 2.0 * std.math.pi * _.radius;
    }

    /// Returns true if point `p` lies inside or on the boundary of the circle.
    fn contains(p: Point2): bool {
        return _.center.distSq(p) <= _.radius * _.radius + GEOM_EPS;
    }

    /// Returns true if this circle overlaps or touches `other`.
    fn intersects(other: Circle2): bool {
        const r_sum = _.radius + other.radius;
        return _.center.distSq(other.center) <= r_sum * r_sum + GEOM_EPS;
    }
}

fn pointLess(a: Point2, b: Point2): bool {
    if (@abs(a.x - b.x) > GEOM_EPS) {
        return a.x < b.x;
    }
    return a.y < b.y;
}

fn siftDown(pts: []Point2, start: usize, end: usize) {
    let root = start;
    while (root * 2 + 1 < end) {
        let child = root * 2 + 1;
        if (child + 1 < end && pointLess(pts[child], pts[child + 1])) {
            child += 1;
        }
        if (pointLess(pts[root], pts[child])) {
            const tmp = pts[root];
            pts[root] = pts[child];
            pts[child] = tmp;
            root = child;
        } else {
            return;
        }
    }
}

fn sortPoints(pts: []Point2) {
    if (pts.len <= 1) return;
    let i = pts.len / 2;
    while (i > 0) {
        i -= 1;
        siftDown(pts, i, pts.len);
    }
    let end = pts.len;
    while (end > 1) {
        end -= 1;
        const tmp = pts[0];
        pts[0] = pts[end];
        pts[end] = tmp;
        siftDown(pts, 0, end);
    }
}

fn clamp01(v: f64): f64 {
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

fn clampRange(v: f64, lo: f64, hi: f64): f64 {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

fn crossOrient(p: Point2, q: Point2, r: Point2): f64 {
    return (q.x - p.x) * (r.y - p.y) - (q.y - p.y) * (r.x - p.x);
}

fn onCollinearSegment(p: Point2, q: Point2, r: Point2): bool {
    const min_x = if (p.x < r.x) p.x else r.x;
    const max_x = if (p.x > r.x) p.x else r.x;
    const min_y = if (p.y < r.y) p.y else r.y;
    const max_y = if (p.y > r.y) p.y else r.y;
    return q.x >= min_x - GEOM_EPS &&
        q.x <= max_x + GEOM_EPS &&
        q.y >= min_y - GEOM_EPS &&
        q.y <= max_y + GEOM_EPS;
}

/// Stateless namespace for 2D computational geometry algorithms.
pub static class Geom2D {
    /// Determines the orientation of ordered triplet `(p, q, r)`.
    fn orientation(p: Point2, q: Point2, r: Point2): Orientation {
        const val = crossOrient(p, q, r);
        if (@abs(val) <= GEOM_EPS) return .Collinear;
        if (val < 0.0) return .Clockwise;
        return .CounterClockwise;
    }

    /// Returns true if segments `s1` and `s2` intersect (including endpoint touch and collinear overlap).
    fn segmentsIntersect(s1: Segment2, s2: Segment2): bool {
        const box1 = Aabb2.init(s1.a.x, s1.a.y, s1.b.x, s1.b.y);
        const box2 = Aabb2.init(s2.a.x, s2.a.y, s2.b.x, s2.b.y);
        if (!box1.intersects(box2)) return false;

        const o1 = Geom2D.orientation(s1.a, s1.b, s2.a);
        const o2 = Geom2D.orientation(s1.a, s1.b, s2.b);
        const o3 = Geom2D.orientation(s2.a, s2.b, s1.a);
        const o4 = Geom2D.orientation(s2.a, s2.b, s1.b);

        if (o1 != o2 && o3 != o4) return true;
        if (o1 == .Collinear && onCollinearSegment(s1.a, s2.a, s1.b)) return true;
        if (o2 == .Collinear && onCollinearSegment(s1.a, s2.b, s1.b)) return true;
        if (o3 == .Collinear && onCollinearSegment(s2.a, s1.a, s2.b)) return true;
        if (o4 == .Collinear && onCollinearSegment(s2.a, s1.b, s2.b)) return true;

        return false;
    }

    /// Computes an intersection point of segments `s1` and `s2`, or `null` if disjoint.
    /// For collinear overlapping segments, returns the lexicographically smallest shared endpoint.
    fn segmentIntersectionPoint(s1: Segment2, s2: Segment2): Point2? {
        if (!Geom2D.segmentsIntersect(s1, s2)) return null;

        const r = s1.b.sub(s1.a);
        const s = s2.b.sub(s2.a);
        const denom = r.cross(s);
        const qp = s2.a.sub(s1.a);

        if (@abs(denom) > GEOM_EPS) {
            const t_raw = qp.cross(s) / denom;
            const u_raw = qp.cross(r) / denom;
            if (t_raw >= -GEOM_EPS && t_raw <= 1.0 + GEOM_EPS &&
                u_raw >= -GEOM_EPS && u_raw <= 1.0 + GEOM_EPS)
            {
                const t = clamp01(t_raw);
                return Point2 {
                    x: s1.a.x + t * r.x,
                    y: s1.a.y + t * r.y,
                };
            }
            return null;
        }

        // Parallel or collinear: segmentsIntersect already verified overlap/touch.
        // Pick the lexicographically smallest endpoint among endpoints lying on both segments.
        let best: Point2? = null;
        const candidates: [4]Point2 = [s1.a, s1.b, s2.a, s2.b];
        for (cand of candidates) {
            if (onCollinearSegment(s1.a, cand, s1.b) && onCollinearSegment(s2.a, cand, s2.b)) {
                if (prev := best) {
                    if (pointLess(cand, prev)) {
                        best = cand;
                    }
                } else {
                    best = cand;
                }
            }
        }
        return best;
    }

    /// Computes the shortest Euclidean distance from point `p` to segment `s`.
    fn pointToSegmentDist(p: Point2, s: Segment2): f64 {
        const ab = s.b.sub(s.a);
        const len_sq = ab.normSq();
        if (len_sq <= GEOM_EPS * GEOM_EPS) {
            return p.dist(s.a);
        }
        const ap = p.sub(s.a);
        const t = clamp01(ap.dot(ab) / len_sq);
        const proj = Point2 {
            x: s.a.x + t * ab.x,
            y: s.a.y + t * ab.y,
        };
        return p.dist(proj);
    }

    /// Computes the unsigned area of a simple polygon using the Shoelace formula.
    fn polygonArea(pts: []const Point2): f64 {
        if (pts.len < 3) return 0.0;
        let sum: f64 = 0.0;
        for (i of 0..pts.len) {
            const j = (i + 1) % pts.len;
            sum += pts[i].x * pts[j].y - pts[j].x * pts[i].y;
        }
        return @abs(sum) * 0.5;
    }

    /// Computes the geometric centroid of a non-degenerate simple polygon.
    fn polygonCentroid(pts: []const Point2): Point2 ! GeomError {
        if (pts.len < 3) return GeomError.InsufficientPoints;
        let signed_area2: f64 = 0.0;
        let cx_sum: f64 = 0.0;
        let cy_sum: f64 = 0.0;
        for (i of 0..pts.len) {
            const j = (i + 1) % pts.len;
            const c = pts[i].x * pts[j].y - pts[j].x * pts[i].y;
            signed_area2 += c;
            cx_sum += (pts[i].x + pts[j].x) * c;
            cy_sum += (pts[i].y + pts[j].y) * c;
        }
        if (@abs(signed_area2) <= GEOM_EPS) {
            return GeomError.DegenerateGeometry;
        }
        const factor = 1.0 / (3.0 * signed_area2);
        return Point2 {
            x: cx_sum * factor,
            y: cy_sum * factor,
        };
    }

    /// Returns true if point `p` lies inside or on the boundary of polygon `pts` (ray-casting + edge check).
    fn pointInPolygon(p: Point2, pts: []const Point2): bool {
        if (pts.len < 3) return false;
        let inside = false;
        for (i of 0..pts.len) {
            const j = (i + 1) % pts.len;
            const vi = pts[i];
            const vj = pts[j];
            if (Geom2D.pointToSegmentDist(p, Segment2.init(vi, vj)) <= GEOM_EPS) {
                return true;
            }
            const straddles = (vi.y > p.y) != (vj.y > p.y);
            if (straddles) {
                const intersect_x = (vj.x - vi.x) * (p.y - vi.y) / (vj.y - vi.y) + vi.x;
                if (p.x < intersect_x) {
                    inside = !inside;
                }
            }
        }
        return inside;
    }

    /// Computes the counter-clockwise convex hull of `pts` using Andrew's Monotone Chain O(N log N) algorithm.
    /// Filters interior and collinear boundary points; returns `DegenerateGeometry` if all points are collinear.
    fn convexHull(alloc: std.mem.Allocator, pts: []const Point2): []Point2 ! GeomError {
        if (pts.len < 3) return GeomError.InsufficientPoints;

        let sorted = alloc.alloc(Point2, pts.len) catch return GeomError.OutOfMemory;
        defer alloc.free(sorted);
        @memcpy(sorted, pts);
        sortPoints(sorted);

        // Deduplicate identical points within GEOM_EPS
        let unique_len: usize = 0;
        for (i of 0..sorted.len) {
            if (unique_len == 0 || !sorted[i].equals(sorted[unique_len - 1], GEOM_EPS)) {
                sorted[unique_len] = sorted[i];
                unique_len += 1;
            }
        }
        if (unique_len < 3) return GeomError.DegenerateGeometry;

        let work = alloc.alloc(Point2, unique_len * 2) catch return GeomError.OutOfMemory;
        defer alloc.free(work);

        let k: usize = 0;
        // Lower hull
        for (i of 0..unique_len) {
            const p = sorted[i];
            while (k >= 2 && crossOrient(work[k - 2], work[k - 1], p) <= GEOM_EPS) {
                k -= 1;
            }
            work[k] = p;
            k += 1;
        }

        // Upper hull
        const lower_len = k;
        let i_rev = unique_len - 1;
        while (i_rev > 0) {
            i_rev -= 1;
            const p = sorted[i_rev];
            while (k > lower_len && crossOrient(work[k - 2], work[k - 1], p) <= GEOM_EPS) {
                k -= 1;
            }
            work[k] = p;
            k += 1;
        }

        // Last point duplicates work[0]
        const hull_len = if (k > 0) k - 1 else 0;
        if (hull_len < 3) return GeomError.DegenerateGeometry;

        let res = alloc.alloc(Point2, hull_len) catch return GeomError.OutOfMemory;
        @memcpy(res, work[0..hull_len]);
        return res;
    }

    /// Returns true if two axis-aligned bounding boxes overlap or touch.
    fn aabbIntersects(a: Aabb2, b: Aabb2): bool {
        return a.intersects(b);
    }

    /// Returns true if two circles overlap or touch.
    fn circleIntersects(c1: Circle2, c2: Circle2): bool {
        return c1.intersects(c2);
    }

    /// Returns true if circle `c` overlaps or touches axis-aligned bounding box `box`.
    fn circleAabbIntersects(c: Circle2, box: Aabb2): bool {
        const closest_x = clampRange(c.center.x, box.min_x, box.max_x);
        const closest_y = clampRange(c.center.y, box.min_y, box.max_y);
        const dx = c.center.x - closest_x;
        const dy = c.center.y - closest_y;
        return dx * dx + dy * dy <= c.radius * c.radius + GEOM_EPS;
    }
}
