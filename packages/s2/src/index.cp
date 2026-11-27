pub const S2_MAX_LEVEL: u8 = 30;
pub const S2_NUM_FACES: u8 = 6;
pub const EARTH_RADIUS_METERS: f64 = 6371010.0;
pub const PI: f64 = 3.14159265358979323846;

pub struct LatLng {
    pub lat_deg: f64,
    pub lng_deg: f64,

    pub static fn fromDegrees(lat: f64, lng: f64): LatLng {
        return LatLng { lat_deg: lat, lng_deg: lng };
    }

    pub fn isValid(): bool {
        return _.lat_deg >= -90.0 && _.lat_deg <= 90.0 && _.lng_deg >= -180.0 && _.lng_deg <= 180.0;
    }

    pub fn latRad(): f64 {
        return _.lat_deg * PI / 180.0;
    }

    pub fn lngRad(): f64 {
        return _.lng_deg * PI / 180.0;
    }
}

fn s2TrailingZeros(v: u64): u8 {
    if (v == 0) { return 64; }
    let n: u8 = 0;
    let x = v;
    while ((x & 1) == 0) {
        n = n + 1;
        x = x >> 1;
    }
    return n;
}

fn s2MaskAbove(lsb_val: u64): u64 {
    let all_ones: u64 = ((4294967295 as u64) << 32) | (4294967295 as u64);
    let low_mask: u64 = (lsb_val << 1) - 1;
    return all_ones ^ low_mask;
}

pub struct S2CellId {
    pub id: u64,

    pub static fn fromFace(face_id: u8): S2CellId? {
        if (face_id >= S2_NUM_FACES) { return null; }
        let f = (face_id as u64) << 61;
        let lsb_bit = (1 as u64) << 60;
        return S2CellId { id: f | lsb_bit };
    }

    pub static fn fromFacePosLevel(face_id: u8, pos: u64, lvl: u8): S2CellId? {
        if (face_id >= S2_NUM_FACES || lvl > S2_MAX_LEVEL) { return null; }
        let shift: u64 = ((S2_MAX_LEVEL - lvl) as u64) * 2;
        let lsb_bit: u64 = (1 as u64) << shift;
        let masked_pos = (pos & (((1 as u64) << 60) - 1)) & s2MaskAbove(lsb_bit);
        let f = (face_id as u64) << 61;
        return S2CellId { id: f | masked_pos | lsb_bit };
    }

    pub fn face(): u8 {
        return (_.id >> 61) as u8;
    }

    pub fn lsb(): u64 {
        if (_.id == 0) { return 0; }
        let tz = s2TrailingZeros(_.id);
        return (1 as u64) << (tz as u64);
    }

    pub fn isValid(): bool {
        if (_.id == 0) { return false; }
        if (_.face() >= S2_NUM_FACES) { return false; }
        let tz = s2TrailingZeros(_.id);
        return (tz & 1) == 0 && tz <= 60;
    }

    pub fn level(): u8 {
        let tz = s2TrailingZeros(_.id);
        return S2_MAX_LEVEL - (tz / 2);
    }

    pub fn isLeaf(): bool {
        return (_.id & 1) != 0;
    }

    pub fn parent(target_lvl: u8): S2CellId? {
        if (!_.isValid() || target_lvl > _.level()) { return null; }
        let new_shift: u64 = ((S2_MAX_LEVEL - target_lvl) as u64) * 2;
        let new_lsb: u64 = (1 as u64) << new_shift;
        return S2CellId { id: (_.id & s2MaskAbove(new_lsb)) | new_lsb };
    }

    pub fn child(quadrant: u8): S2CellId? {
        if (!_.isValid() || quadrant > 3 || _.level() >= S2_MAX_LEVEL) {
            return null;
        }
        let child_shift: u64 = ((S2_MAX_LEVEL - (_.level() + 1)) as u64) * 2;
        let child_lsb: u64 = (1 as u64) << child_shift;
        let cleared = _.id & s2MaskAbove(_.lsb());
        let qbits = (quadrant as u64) << (child_shift + 1);
        return S2CellId { id: cleared | qbits | child_lsb };
    }

    pub fn rangeMin(): u64 {
        return _.id - (_.lsb() - 1);
    }

    pub fn rangeMax(): u64 {
        return _.id + (_.lsb() - 1);
    }

    pub fn contains(other: S2CellId): bool {
        return other.id >= _.rangeMin() && other.id <= _.rangeMax();
    }
}

fn s2Sin(x_in: f64): f64 {
    let x = x_in;
    // Reduce to [-PI, PI]
    while (x > PI) { x = x - 2.0 * PI; }
    while (x < -PI) { x = x + 2.0 * PI; }
    let term = x;
    let sum = x;
    let x2 = x * x;
    let k: usize = 1;
    while (k <= 10) {
        let d1 = (2 * k) as f64;
        let d2 = (2 * k + 1) as f64;
        term = -term * x2 / (d1 * d2);
        sum = sum + term;
        k = k + 1;
    }
    return sum;
}

fn s2Cos(x_in: f64): f64 {
    return s2Sin(x_in + PI * 0.5);
}

fn s2Sqrt(x: f64): f64 {
    if (x <= 0.0) { return 0.0; }
    let g: f64 = if (x >= 1.0) { x } else { 1.0 };
    let i: usize = 0;
    while (i < 48) {
        g = 0.5 * (g + x / g);
        i = i + 1;
    }
    return g;
}

fn s2Asin(x: f64): f64 {
    if (x >= 1.0) { return PI * 0.5; }
    if (x <= -1.0) { return -PI * 0.5; }
    // Newton iteration on sin(y) = x
    let y = x;
    let i: usize = 0;
    while (i < 16) {
        let c = s2Cos(y);
        if (c < 1.0e-12 && c > -1.0e-12) { break; }
        y = y - (s2Sin(y) - x) / c;
        i = i + 1;
    }
    return y;
}

pub fn haversineMeters(a: LatLng, b: LatLng): f64 {
    let dlat = b.latRad() - a.latRad();
    let dlng = b.lngRad() - a.lngRad();
    let s1 = s2Sin(dlat * 0.5);
    let s2 = s2Sin(dlng * 0.5);
    let h = s1 * s1 + s2Cos(a.latRad()) * s2Cos(b.latRad()) * s2 * s2;
    let clamped = if (h > 1.0) { 1.0 } else { h };
    return 2.0 * EARTH_RADIUS_METERS * s2Asin(s2Sqrt(clamped));
}
