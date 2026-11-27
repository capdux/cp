[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/math-interp

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:lerp` ([https://github.com/mattdesl/lerp](https://github.com/mattdesl/lerp)) / `npm:eases` ([https://github.com/mattdesl/eases](https://github.com/mattdesl/eases)) / `npm:smoothstep` ([https://github.com/mikolalysenko/smoothstep](https://github.com/mikolalysenko/smoothstep)) / `crates.io:interpolation` ([https://github.com/PistonDevelopers/interpolation](https://github.com/PistonDevelopers/interpolation)) / `crates.io:keyframe` ([https://github.com/HannesMann/keyframe](https://github.com/HannesMann/keyframe))（原库名 `lerp` → CP 包名 `@cap/math-interp`）

Zero-heap interpolation, range clamping/remapping, wrap/ping-pong, angle conversion, and easing curves in pure CP.

## API
- `clamp`, `clamp01`, `approxEq`, `minF64`, `maxF64`, `absF64`
- `lerp`, `lerpClamped`, `inverseLerp`, `remap`, `remapClamped`
- `smoothstep`, `smootherstep`, `step`
- `wrap`, `pingPong`, `degToRad`, `radToDeg`
- `easeInQuad`, `easeOutQuad`, `easeInOutQuad`, `easeInCubic`, `easeOutCubic`, `easeInOutCubic`
