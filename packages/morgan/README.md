[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/morgan-log

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:morgan` ([https://github.com/expressjs/morgan](https://github.com/expressjs/morgan))（原库名 `morgan` → CP 包名 `@cap/morgan-log`）

Clean-room MIT, zero-heap HTTP access log template formatting engine for CP — modeled on npm `morgan` v1.10.

## Dependencies
- `@cap/itoa` (`1.0.0`) — reused for zero-heap decimal formatting of HTTP status codes, content lengths, and millisecond response times (`formatU64`).

## API

### Preset Format Constants
- `MORGAN_TINY`: `":method :url :status :res[content-length] - :response-time ms"`
- `MORGAN_DEV`: `":method :url :status :response-time ms - :res[content-length]"`
- `MORGAN_SHORT`: `":remote-addr :remote-user :method :url HTTP/:http-version :status :res[content-length] - :response-time ms"`
- `MORGAN_COMMON`: `":remote-addr - :remote-user [:date] \":method :url HTTP/:http-version\" :status :res[content-length]"`
- `MORGAN_COMBINED`: `":remote-addr - :remote-user [:date] \":method :url HTTP/:http-version\" :status :res[content-length] \":referrer\" \":user-agent\""`

### Structs
- `HttpLogEvent { remote_addr, remote_user, date, method, url, http_version, status, content_length, has_content_length, referrer, user_agent, response_time_us }`

### Functions
- `formatMorganLog(format_str: string, ev: HttpLogEvent, out: []u8): usize`
  - Expands `:remote-addr`, `:remote-user`, `:date`, `:method`, `:url`, `:http-version`, `:status`, `:res[content-length]`, `:referrer`, `:user-agent`, and `:response-time` (`X.YYY` ms from microseconds). Emits `"-"` for absent fields per Apache CLF conventions.
- `formatPreset(preset: string, ev: HttpLogEvent, out: []u8): usize`
  - Accepts `"tiny"`, `"dev"`, `"short"`, `"common"`, `"combined"`, or any custom format string.
