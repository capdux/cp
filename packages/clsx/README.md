[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# clsx

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:clsx` ([https://github.com/lukeed/clsx](https://github.com/lukeed/clsx))

Zero-heap conditional CSS class name builder, whitespace normalizer, and deduplicator in pure CP (npm `clsx` / `classnames` parity).

## Features

- `ClassBuilder`: writes normalized single-space-separated class tokens directly into caller `[]u8` buffer
- `add`, `addIf`, `addUnique`, `addConds`, `has` (exact word-boundary check), `build`, `len`, `isOverflow`, `reset`
- Top-level helpers `joinClasses` and `joinCondClasses`

## Usage

```cp
import { ClassBuilder, CondClass } from "clsx";

let buf: [128]u8 = undefined;
let b = ClassBuilder.init(buf[0..128]);
* = b.add("  btn   btn-lg ");
* = b.addIf(true, "active");
* = b.addUnique("btn text-white");
// b.build() => "btn btn-lg active text-white"
```
