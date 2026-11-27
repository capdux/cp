[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# badge-maker

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:badge-maker` ([https://github.com/badges/shields](https://github.com/badges/shields))（原库名 `badge-maker` → CP 包名 `badge-maker`）

Zero-heap Shields.io-style SVG status badge generator in pure CP (replaces npm `badge-maker` / `gh-badges`).

Supports `Flat`, `FlatSquare`, and `Plastic` styles, 15+ Shields.io named colors (`brightgreen`, `success`, `critical`, `informational`, etc.), dynamic Verdana 11px width estimation, and full XML/XSS escaping.

## Install

```bash
cap add badge-maker
```

## Usage

```cp
import { makeBadge, renderBadge, BadgeOptions, BadgeStyle } from "badge-maker";

let buf: [2048]u8 = undefined;
const svg = makeBadge("build", "passing", "brightgreen", buf[0..2048]);
```

## API

| Function | Description |
| --- | --- |
| `makeBadge(label, message, color, out)` | Render a standard `Flat` SVG badge |
| `renderBadge(opts, out)` | Render a `Flat`, `FlatSquare`, or `Plastic` SVG badge |
| `defaultBadge(label, message, color)` | Create default `BadgeOptions` |
| `resolveBadgeColor(name)` | Resolve Shields.io named color to `#hex` |
| `estimateTextWidth(s)` | Estimate Verdana 11px text width in pixels |
