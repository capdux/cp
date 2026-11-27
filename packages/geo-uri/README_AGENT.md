# geo-uri@1.0.0 (Agent Quickref)

> Zero-allocation RFC 5870 geo: URI parser, coordinate validator, and formatter for CP. | License: MIT
> Import: `import { ... } from "geo-uri";` | Full types: `src/index.d.cp`

## Functions
- `export fn isValidLatLng(lat: f64, lng: f64): bool;`
- `export fn parseGeoUri(uri: string): GeoUri;`
- `export fn formatGeoUri(lat: f64, lng: f64, has_alt: bool, alt: f64, has_u: bool, u: f64, out: []u8): usize;`
