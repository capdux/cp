# @cap/plotters-svg

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:plotters-svg` ([https://github.com/plotters-rs/plotters](https://github.com/plotters-rs/plotters)) — `MIT`

Zero-heap pure CP SVG vector graphics drawing backend inspired by Rust `plotters-svg`.

## Features

- `SvgColor` (`r`, `g`, `b`, plus `rgb`, `black`, `white`, `red`, `green`, `blue`)
- `SvgCanvas` writing directly into caller-provided `[]u8` buffer with overflow tracking:
  - `begin(width, height)` / `end(): usize`
  - `drawRect(x, y, w, h, fill, stroke_width)`
  - `drawLine(x1, y1, x2, y2, stroke, stroke_width)`
  - `drawCircle(cx, cy, r, fill)`
  - `drawPolyline(xs, ys, stroke, stroke_width)`
  - `drawText(x, y, text, font_size, fill)` with automatic XML escaping (`<`, `>`, `&`, `"`, `'`)

## License

MIT
