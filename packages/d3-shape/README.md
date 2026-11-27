[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# d3-shape

Zero-allocation SVG shape and pie chart path generator in pure CP.

## API

- `Point2D { x: f64, y: f64 }`
- `PieSlice { value: f64, index: usize, start_angle: f64, end_angle: f64, pad_angle: f64 }`
- `computePie(values: []const f64, start_angle: f64, end_angle: f64, pad_angle: f64, out: []PieSlice): usize`
- `linePath(pts: []const Point2D, out: []u8): usize`
- `stepLinePath(pts: []const Point2D, out: []u8): usize`
- `arcPath(inner_r: f64, outer_r: f64, start_angle: f64, end_angle: f64, out: []u8): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`d3-shape@3.2.0`)
- **Upstream Repository**: https://github.com/d3/d3-shape
- **License**: ISC (clean-room CP implementation)
