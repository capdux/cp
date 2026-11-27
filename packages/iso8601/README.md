[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/iso8601`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:iso8601` ([https://github.com/badboy/iso8601](https://github.com/badboy/iso8601))

Zero-heap ISO-8601 date (calendar/week/ordinal), time, timezone offset, datetime, and duration parser & formatter — clean-room CP equivalent of crates.io `iso8601`.

## Features
- `parseDate` (supports `YYYY-MM-DD`, `YYYYMMDD`, week dates `YYYY-Www-D`, ordinal dates `YYYY-DDD`, leap-year validation)
- `parseTime` (supports `HH:MM:SS.sss`, `Z`, `±HH:MM`, `±HHMM`, leap second `60`)
- `parseDateTime` & `formatDateTime`
- `parseDuration` (supports `P1Y2M3DT4H5M6.789S`, `P2W`)
- `isLeapYear`, `daysInMonth`
