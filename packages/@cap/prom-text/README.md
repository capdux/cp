# @cap/prom-text

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:prom-client` ([https://github.com/siimon/prom-client](https://github.com/siimon/prom-client))（原库名 `prom-client` → CP 包名 `@cap/prom-text`）

Zero-heap Prometheus / OpenMetrics `/metrics` text exposition format serializer in pure CP.

## Features

- Strict validation: `isValidMetricName` (`[a-zA-Z_:][a-zA-Z0-9_:]*`) and `isValidLabelName` (`[a-zA-Z_][a-zA-Z0-9_]*`)
- `PromWriter`: writes `# HELP`, `# TYPE` (`counter`/`gauge`/`histogram`/`summary`), `writeSampleInt`, and `writeSampleWithTs` into caller `[]u8` buffer
- Automatic escaping of `\`, `"`, and `\n` in label values and `\`, `\n` in `# HELP` lines

## Usage

```cp
import { PromWriter, Label, METRIC_COUNTER } from "@cap/prom-text";

let buf: [256]u8 = undefined;
let w = PromWriter.init(buf[0..256]);
* = w.writeHelp("http_requests_total", "Total HTTP requests");
* = w.writeType("http_requests_total", METRIC_COUNTER);
const labels: [1]Label = [Label { key: "method", val: "GET" }];
* = w.writeSampleInt("http_requests_total", labels[0..1], 42);
```
