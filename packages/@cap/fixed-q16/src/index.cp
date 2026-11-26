// @cap/fixed-q16 — Zero-heap deterministic Q16.16 fixed-point arithmetic
// Clean-room MIT implementation in pure CP.

pub const FRAC_BITS: i32 = 16;
pub const ONE_RAW: i32 = 65536;
pub const HALF_RAW: i32 = 32768;

pub struct Fixed16 {
    pub raw: i32,

    pub static fn fromRaw(raw: i32): Fixed16 {
        return Fixed16 { raw: raw };
    }

    pub static fn fromInt(v: i32): Fixed16 {
        return Fixed16 { raw: v * ONE_RAW };
    }

    pub static fn fromF64(v: f64): Fixed16 {
        let scaled = v * 65536.0;
        if (scaled >= 0.0) {
            return Fixed16 { raw: (scaled + 0.5) as i32 };
        }
        return Fixed16 { raw: (scaled - 0.5) as i32 };
    }

    pub static fn zero(): Fixed16 {
        return Fixed16 { raw: 0 };
    }

    pub static fn one(): Fixed16 {
        return Fixed16 { raw: ONE_RAW };
    }

    pub fn toF64(): f64 {
        return (_.raw as f64) / 65536.0;
    }

    pub fn floorInt(): i32 {
        return _.raw >> 16;
    }

    pub fn ceilInt(): i32 {
        let rem = _.raw & 65535;
        let fl = _.raw >> 16;
        if (rem == 0) { return fl; }
        return fl + 1;
    }

    pub fn roundInt(): i32 {
        return (_.raw + HALF_RAW) >> 16;
    }

    pub fn add(other: Fixed16): Fixed16 {
        return Fixed16 { raw: _.raw + other.raw };
    }

    pub fn sub(other: Fixed16): Fixed16 {
        return Fixed16 { raw: _.raw - other.raw };
    }

    pub fn neg(): Fixed16 {
        return Fixed16 { raw: -_.raw };
    }

    pub fn abs(): Fixed16 {
        if (_.raw < 0) { return Fixed16 { raw: -_.raw }; }
        return Fixed16 { raw: _.raw };
    }

    pub fn mul(other: Fixed16): Fixed16 {
        let prod: i64 = (_.raw as i64) * (other.raw as i64);
        return Fixed16 { raw: (prod >> 16) as i32 };
    }

    pub fn div(other: Fixed16): Fixed16 {
        if (other.raw == 0) { return Fixed16 { raw: 0 }; }
        let num: i64 = (_.raw as i64) << 16;
        let den: i64 = other.raw as i64;
        let is_neg = (num < 0) != (den < 0);
        let unum: u64 = if (num < 0) { (-num) as u64 } else { num as u64 };
        let uden: u64 = if (den < 0) { (-den) as u64 } else { den as u64 };
        let q: u64 = unum / uden;
        if (is_neg) {
            return Fixed16 { raw: -(q as i32) };
        }
        return Fixed16 { raw: q as i32 };
    }

    pub fn clamp(lo: Fixed16, hi: Fixed16): Fixed16 {
        if (_.raw < lo.raw) { return lo; }
        if (_.raw > hi.raw) { return hi; }
        return Fixed16 { raw: _.raw };
    }

    pub fn lerp(other: Fixed16, t: Fixed16): Fixed16 {
        let diff = other.sub(Fixed16 { raw: _.raw });
        return Fixed16 { raw: _.raw }.add(diff.mul(t));
    }
}
