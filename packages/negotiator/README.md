[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/negotiator

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:negotiator` ([https://github.com/jshttp/negotiator](https://github.com/jshttp/negotiator))

Clean-room MIT, zero-heap RFC 7231 HTTP Content Negotiation library for CP — modeled on npm's `negotiator` v0.6.

## Dependencies
- `@cap/unicase` (`1.0.0`) — case-insensitive token and language tag matching.

## API
- `parseQValue(raw): u32` (`0..1000` milli-units)
- `parseWeightedList(header, out): usize` (sorted descending by `q`)
- `negotiateEncoding(accept_encoding, available): string?`
- `negotiateLanguage(accept_language, available): string?`
- `negotiateCharset(accept_charset, available): string?`
