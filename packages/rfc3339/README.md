[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `rfc3339`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rfc3339` ([https://github.com/chronotope/chrono](https://github.com/chronotope/chrono))

Zero-heap strict RFC 3339 timestamp parser, Unix epoch converter, and UTC formatter — clean-room CP package reusing `@cap/iso8601`.

## Features
- `parseRfc3339(s)` — strict RFC 3339 validation (requires `YYYY-MM-DD[T|t| ]HH:MM:SS(.frac)?(Z|±HH:MM)`) + exact `unix_seconds`
- `isValidRfc3339(s)`
- `daysFromCivil(y, m, d)` & `civilFromDays(days)` — O(1) Howard Hinnant Gregorian algorithms
- `fromUnixSeconds(unix_secs)` & `formatRfc3339Utc(unix_secs, out)`
