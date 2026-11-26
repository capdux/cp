# @cap/statsd

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:hot-shots` ([https://github.com/brightcove/hot-shots](https://github.com/brightcove/hot-shots))（原库名 `hot-shots` → CP 包名 `@cap/statsd`）

Zero-heap StatsD and DogStatsD UDP metric packet encoder/decoder in pure CP.

## Features

- Supports Counter (`|c`), Timing (`|ms`), Gauge (`|g`), and Set (`|s`)
- Optional sample rate (`|@0.25`, omitted at full `1000` milli-rate) and DogStatsD tags (`|#env:prod,svc:api`)
- Zero-copy `parseMetric(packet)` and validated `formatMetric(...)`

## Usage

```cp
import { formatMetric, parseMetric, STATSD_COUNTER } from "@cap/statsd";

let buf: [128]u8 = undefined;
const n = formatMetric("http.req", 1, STATSD_COUNTER, 500, "env:prod", buf[0..128]);
// buf[0..n] => "http.req:1|c|@0.5|#env:prod"
```
