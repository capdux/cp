# `@cap/d3-scale`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-scale` ([https://github.com/d3/d3-scale](https://github.com/d3/d3-scale))

Zero-heap-allocation D3 quantitative scales for data visualization and normalization in pure CP.

## Features

- **`LinearScale`**: Forward mapping (`apply`), inverse mapping (`invert`), optional range clamping (`clamp`), and 1-2-5 nice tick generation (`ticksCount`, `ticks`).
- **`PowScale` & `SqrtScale`**: Signed power scaling supporting negative domains with exact inverse.
- **`LogScale`**: Configurable logarithm base (`base`, e.g., 10 or 2) for positive or negative domains.
- **`QuantizeScale`**: Uniform bucket mapping (`applyIndex`, `applyValue`, `invertExtentMin/Max`).

## API

```cp
import {
    LinearScale, linearScale,
    PowScale, powScale,
    SqrtScale, sqrtScale,
    LogScale, logScale,
    QuantizeScale, quantizeScale,
} from "@cap/d3-scale";
```

## License

ISC
