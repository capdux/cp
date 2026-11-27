[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/geo-uri

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:geo-uri` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

Zero-allocation RFC 5870 `geo:` URI parser, coordinate validator, and formatter for CP.

## Features

- **Zero heap allocation**: parses and formats directly over `string` / `[]u8` buffers.
- **Full RFC 5870 support**: 2D (`geo:lat,lng`), 3D altitude (`geo:lat,lng,alt`), CRS (`;crs=wgs84`), and uncertainty radius (`;u=35.5`).
- **Strict bounds validation**: enforces `-90.0 <= lat <= 90.0`, `-180.0 <= lng <= 180.0`, and `u >= 0.0`.

## Usage

```cp
import { parseGeoUri, formatGeoUri } from "@cap/geo-uri";

const g = parseGeoUri("geo:37.786971,-122.399677,15.5;crs=wgs84;u=35");
let buf: [64]u8 = undefined;
const n = formatGeoUri(g.lat, g.lng, g.has_alt, g.alt, g.has_uncertainty, g.uncertainty, buf[0..64]);
```
