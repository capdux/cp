[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/media-typer

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:media-typer` ([https://github.com/jshttp/media-typer](https://github.com/jshttp/media-typer))

Clean-room MIT, zero-heap RFC 6838 Media Type parser, validator, and formatter for CP — modeled on npm's `media-typer` v1.1.

## Dependencies
- `@cap/unicase` (`1.0.0`) — lowercase normalization and case-insensitive equality.

## API
- `parseMediaType(raw): MediaType` (`valid`, `main_type`, `subtype`, `suffix`)
- `formatMediaType(main_type, subtype, suffix, out): usize`
- `testMediaType(raw): bool`
- `mediaTypeEq(a, b): bool`
