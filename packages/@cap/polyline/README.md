# @cap/polyline

Zero-heap Google Encoded Polyline Algorithm Format encoder and decoder for CP.
Compatible with npm `@mapbox/polyline` and Rust `polyline` (supports both precision 5 and precision 6).

## API

- `LatLng { lat: f64, lng: f64 }`
- `encodeSigned(val: i64, *out: []u8) usize`
- `decodeSigned(data: string, start: usize, *out_val: i64) usize`
- `encodePoints(points: []const LatLng, precision: u32, *out: []u8) usize`
- `decodePoints(encoded: string, precision: u32, *out: []LatLng) usize`
- `countPoints(encoded: string) usize`
- `isValid(encoded: string) bool`

## License

MIT
