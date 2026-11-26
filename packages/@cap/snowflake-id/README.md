# @cap/snowflake-id

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:snowflake` ([https://github.com/capdux/cp](https://github.com/capdux/cp))（原库名 `snowflake` → CP 包名 `@cap/snowflake-id`）

Clean-room MIT, zero-heap 64-bit Snowflake distributed ID generator, decoder, and decimal formatter for CP.

## Dependencies
- `@cap/itoa` (`1.0.0`) — fast zero-heap decimal ID formatting.

## API
- Constants: `TWITTER_EPOCH_MS`, `DISCORD_EPOCH_MS`, `SF_MAX_DATACENTER`, `SF_MAX_WORKER`, `SF_MAX_SEQUENCE`
- `encodeSnowflake`, `decodeSnowflake`, `formatSnowflake`, `parseSnowflake`
- `SnowflakeGenerator` (`new`, `nextId`)
