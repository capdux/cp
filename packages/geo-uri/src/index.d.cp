// Type definitions for CP

export class GeoUri {
    lat: f64,
    lng: f64,
    has_alt: bool,
    alt: f64,
    has_uncertainty: bool,
    uncertainty: f64,
    crs: string,
    valid: bool,
}

export fn guToLower(c: u8): u8;
export fn guEqIgnoreCase(a: &[]u8, b: &[]u8): bool;
export fn guParseF64(s: &[]u8, out: &mut f64): bool;
export fn guWriteRaw(out: []u8, pos: &mut usize, src: &[]u8): bool;
export fn guWriteU64(out: []u8, pos: &mut usize, val: u64): bool;
export fn guWriteF64(out: []u8, pos: &mut usize, val: f64): bool;
export fn isValidLatLng(lat: f64, lng: f64): bool;
export fn parseGeoUri(uri: string): GeoUri;
export fn formatGeoUri(lat: f64, lng: f64, has_alt: bool, alt: f64, has_u: bool, u: f64, out: []u8): usize;
