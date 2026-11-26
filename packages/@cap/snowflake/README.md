# @cap/snowflake

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@sapphire/snowflake` ([https://github.com/sapphiredev/utilities](https://github.com/sapphiredev/utilities)) / `crates.io:sonyflake` ([https://github.com/sony/sonyflake](https://github.com/sony/sonyflake))（原库名 `@sapphire/snowflake` → CP 包名 `@cap/snowflake`）

Zero-heap 64-bit Twitter / Discord Snowflake distributed unique ID generator, decomposer, clock-rollback guard, and decimal formatter in pure CP.

## Features

- Standard 64-bit layout: `1-bit 0 + 41-bit ms delta + 5-bit datacenter + 5-bit worker + 12-bit sequence`
- Built-in `TWITTER_EPOCH_MS` & `DISCORD_EPOCH_MS` constants
- `compose` & `decompose` for lossless packing/unpacking
- `initGenerator` & `nextId(&gen, now_ms)` with monotonic sequence increment, clock-rollback protection (`-1`), and same-ms 4096 overflow signal (`-2`)
- `formatId` & `parseId` for exact decimal string representation without 53-bit float precision loss

## Usage

```cp
import { initGenerator, nextId, TWITTER_EPOCH_MS } from "@cap/snowflake";

let gen = initGenerator(TWITTER_EPOCH_MS, 1, 2);
const id = nextId(&gen, 1732620000000);
```
