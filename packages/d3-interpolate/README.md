[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# d3-interpolate

Zero-allocation numeric, integer, discrete, RGB/HSL color, piecewise, and uniform cubic B-spline interpolators in pure CP.

## API

- `interpolateNumber(a, b, t)` / `interpolateRound(a, b, t)` / `interpolateDiscrete(values, t)`
- `interpolateRgb(c0, c1, t)` / `interpolateHue(a, b, t)` / `interpolateHsl(c0, c1, t)`
- `basis(t, v0, v1, v2, v3)` / `interpolateBasis(values, t)` / `interpolatePiecewise(values, t)`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`d3-interpolate@3.0.1`)
- **Upstream Repository**: https://github.com/d3/d3-interpolate
- **License**: ISC (clean-room CP implementation)
