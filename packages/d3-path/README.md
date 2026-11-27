[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `d3-path`

> **Source Attribution**: `npm:d3-path` ([d3/d3-path](https://github.com/d3/d3-path)) — License: MIT (upstream: ISC)

Zero-allocation Canvas-to-SVG path string serializer (`moveTo`, `lineTo`, `quadraticCurveTo`, `bezierCurveTo`, `rect`, `closePath`) in pure CP.

## Zero-Cost Migration (TS → CP)

Keep the exact upstream package name (`d3-path`) — no renaming required.

```typescript
// TypeScript / Node.js (npm: d3-path)
import { path } from "d3-path";
const p = path();
p.moveTo(0, 0);
p.lineTo(100, 50);
p.closePath();
const d = p.toString(); // "M0,0L100,50Z"
```

```cp
// CP (zero-heap fixed 2048-byte internal buffer)
import { SvgPath } from "d3-path";

let p = SvgPath.new();
p.moveTo(0.0, 0.0);
p.lineTo(100.0, 50.0);
p.closePath();

let out: [128]u8 = undefined;
let n = p.writeTo(out[0..128]);
let d = out[0..n] as string; // "M0,0L100,50Z"
```

## Installation

Add `d3-path` to your `cap.json`:

```json
{
  "dependencies": {
    "d3-path": "1.0.0"
  }
}
```

## API Overview

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `SvgPath.new` | `(): SvgPath` | Create an empty path builder (2048-byte stack buffer) |
| `SvgPath.moveTo` | `(x: f64, y: f64)` | Emit `M x,y` and start a new subpath |
| `SvgPath.lineTo` | `(x: f64, y: f64)` | Emit `L x,y` (or `M x,y` if no subpath started) |
| `SvgPath.quadraticCurveTo` | `(cpx: f64, cpy: f64, x: f64, y: f64)` | Emit quadratic Bézier curve `Q cpx,cpy,x,y` |
| `SvgPath.bezierCurveTo` | `(cp1x: f64, cp1y: f64, cp2x: f64, cp2y: f64, x: f64, y: f64)` | Emit cubic Bézier curve `C cp1x,cp1y,cp2x,cp2y,x,y` |
| `SvgPath.rect` | `(x: f64, y: f64, w: f64, h: f64)` | Emit axis-aligned rectangle `M x,y h w v h h -w Z` |
| `SvgPath.closePath` | `()` | Emit `Z` and return current point to subpath origin |
| `SvgPath.byteLen` | `(): usize` | Current serialized byte length in `buf` |
| `SvgPath.writeTo` | `(out: []u8): usize` | Copy serialized SVG path string into `out` |
| `SvgPath.reset` | `()` | Clear path state for zero-cost buffer reuse |
| `d3pPushStr` / `d3pPushU64` / `d3pPushNum` | Low-level writers | Direct formatting helpers (up to 3 decimal digits, trailing zeros trimmed) |

## Practical Examples

### 1. Curves, Rectangles & Buffer Reuse

```cp
import { SvgPath } from "d3-path";

let p = SvgPath.new();
p.moveTo(10.0, 20.0);
p.quadraticCurveTo(50.0, 0.0, 90.0, 20.0);
p.bezierCurveTo(100.0, 40.0, 110.0, 60.0, 120.0, 80.0);
p.closePath();

let out: [256]u8 = undefined;
let n1 = p.writeTo(out[0..256]);
let curve_d = out[0..n1] as string;

// Reset and reuse the same SvgPath instance
p.reset();
p.rect(0.0, 0.0, 64.5, 32.25);
let n2 = p.writeTo(out[0..256]);
let rect_d = out[0..n2] as string;
```

## Notes & Edge Cases

- **Fixed stack capacity**: `SvgPath.buf` is `[2048]u8`; check `p.byteLen()` or call `p.reset()` between shapes when serializing many paths.
- **Compact numeric formatting**: Coordinates format with up to 3 decimal places with trailing zeros automatically trimmed (`10.0` → `"10"`, `32.250` → `"32.25"`).
- **Zero heap allocations**: Suitable for real-time SVG/Canvas rendering and server-side chart generation.
