[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/haversine-geo

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:haversine` ([https://github.com/njj/haversine](https://github.com/njj/haversine)) / `crates.io:haversine` ([https://github.com/gavinuhma/haversine](https://github.com/gavinuhma/haversine))（原库名 `haversine` → CP 包名 `@cap/haversine-geo`）

Zero-allocation WGS-84 spherical Haversine great-circle distance, initial bearing, midpoint, and destination point engine for CP (zero external dependencies).

## Features

- **Zero heap allocation**: built-in high-precision pure-CP `sin`, `cos`, `atan2`, `asin`, `sqrt`.
- **Great-circle geometry**: `distanceMeters`, `distanceKm`, `distanceMiles`, `initialBearing` (`[0, 360)`), `midpoint`, `destinationPoint`, and `isValidCoord`.

## Usage

```cp
import { LatLon, distanceKm, initialBearing, destinationPoint } from "@cap/haversine-geo";

const nyc = LatLon{ lat: 40.7128 as f64, lon: -74.0060 as f64 };
const lon = LatLon{ lat: 51.5074 as f64, lon: -0.1278 as f64 };
const km = distanceKm(nyc, lon); // ~5570.2 km
```
