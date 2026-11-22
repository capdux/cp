# `cap:olc` — Zero-Allocation Google Open Location Code (Plus Codes) Geocoder

> [!NOTE]
> 本子模块已归并至统一主包 `cap:geo`，导入请使用 `import { ... } from "cap:geo"`。

Pure CP, zero-heap-allocation implementation of **Google Open Location Code (Plus Codes)** providing Base-20 global grid encoding (padded 2..8, standard 10, and 4×5 refined 11..15), high-precision integer-backed decoding, validation, local shortening, and nearest-quadrant recovery.

## Constants & Types

- `OLC_ALPHABET = "23456789CFGHJMPQRVWX"`
- `OLC_SEPARATOR = '+'`, `OLC_SEPARATOR_POS = 8`, `OLC_PADDING = '0'`, `OLC_DEFAULT_LEN = 10`, `OLC_MAX_LEN = 15`
- `OlcArea`: `{ lat_lo: f64, lng_lo: f64, lat_hi: f64, lng_hi: f64, code_len: usize }` with `latCenter(): f64` and `lngCenter(): f64`

## Core Functions

| Symbol | Description |
| :--- | :--- |
| `isValidOlc(code: []const u8): bool` | Strictly validates `+` placement, `'0'` padding rules, and Base-20 characters (case-insensitive). |
| `isShortOlc(code: []const u8): bool` | Returns `true` if `code` is valid and shortened (`+` at even index `< 8`). |
| `isFullOlc(code: []const u8): bool` | Returns `true` if `code` is valid, full (`+` at index `8`), and within global lat/lng bounds. |
| `olcEncode(lat, lng, code_len, out: *[]u8): usize?` | Encodes coordinates to a Plus Code (`code_len` in `2, 4, 6, 8, 10..15`), clipping lat & normalizing lng. |
| `olcDecode(full_code: []const u8): OlcArea?` | Decodes any valid full Plus Code (padded or grid-refined) to its exact bounding box. |
| `olcShorten(full_code, ref_lat, ref_lng, out: *[]u8): usize?` | Trims 4, 6, or 8 leading characters relative to a nearby reference point. |
| `olcRecoverNearest(short_code, ref_lat, ref_lng, out: *[]u8): usize?` | Recovers the full Plus Code nearest to `(ref_lat, ref_lng)` with half-resolution quadrant adjustment. |

## Example

```cp
import {
    OLC_DEFAULT_LEN,
    olcEncode,
    olcShorten,
    olcRecoverNearest,
} from "cap:geo";

fn roundTrip(full: *[]u8, short_buf: *[]u8, rec: *[]u8) {
    if (fn_len := olcEncode(47.365590, 8.524997, OLC_DEFAULT_LEN, full)) {
        if (sn := olcShorten(full.*[0..fn_len], 47.37, 8.53, short_buf)) {
            * = olcRecoverNearest(short_buf.*[0..sn], 47.37, 8.53, rec);
        }
    }
}
```
