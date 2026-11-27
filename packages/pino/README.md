[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# pino

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pino` ([https://github.com/pinojs/pino](https://github.com/pinojs/pino))（原库名 `pino` → CP 包名 `pino`）

Clean-room MIT, zero-heap structured NDJSON logger for CP — modeled on npm's `pino` v9.

## Dependencies
- `itoa` (`1.0.0`) — fast zero-heap integer serialization.

## API
- Levels: `PINO_TRACE (10)`, `PINO_DEBUG (20)`, `PINO_INFO (30)`, `PINO_WARN (40)`, `PINO_ERROR (50)`, `PINO_FATAL (60)`, `PINO_SILENT (100)`
- `levelLabel(level): string`
- `PinoLogger` (`new`, `isLevelEnabled`, `bindStr`, `bindInt`, `bindBool`, `formatLine`)
