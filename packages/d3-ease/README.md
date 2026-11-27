[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `d3-ease`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-ease` ([https://github.com/d3/d3-ease](https://github.com/d3/d3-ease))

Zero-heap-allocation D3 easing functions for smooth animation and interpolation in pure CP.

## Features

- **Complete D3 v3 Easing Suite**: `Linear`, `Quad`, `Cubic`, `Poly`, `Sin`, `Exp` (exact v3 normalized `tpmt`), `Circle`, `Bounce`, `Back`, and `Elastic` (`In`, `Out`, `InOut`).
- **Default & Parameterized Variants**: Includes `easePoly*Exp(t, e)`, `easeBack*Overshoot(t, s)`, and `easeElastic*Params(t, a, p)`.
- **Zero Heap Allocation**: Pure stack-only IEEE-754 `f64` math.

## API

```cp
import {
    easeLinear,
    easeQuadIn, easeQuadOut, easeQuadInOut,
    easeCubicIn, easeCubicOut, easeCubicInOut,
    easePolyIn, easePolyOut, easePolyInOut,
    easePolyInExp, easePolyOutExp, easePolyInOutExp,
    easeSinIn, easeSinOut, easeSinInOut,
    easeExpIn, easeExpOut, easeExpInOut,
    easeCircleIn, easeCircleOut, easeCircleInOut,
    easeBounceIn, easeBounceOut, easeBounceInOut,
    easeBackIn, easeBackOut, easeBackInOut,
    easeBackInOvershoot, easeBackOutOvershoot, easeBackInOutOvershoot,
    easeElasticIn, easeElasticOut, easeElasticInOut,
    easeElasticInParams, easeElasticOutParams, easeElasticInOutParams,
} from "d3-ease";
```

## License

ISC
