# @cap/pino-log

Clean-room MIT, zero-heap structured NDJSON logger for CP — modeled on npm's `pino` v9.

## Dependencies
- `@cap/itoa` (`1.0.0`) — fast zero-heap integer serialization.

## API
- Levels: `PINO_TRACE (10)`, `PINO_DEBUG (20)`, `PINO_INFO (30)`, `PINO_WARN (40)`, `PINO_ERROR (50)`, `PINO_FATAL (60)`, `PINO_SILENT (100)`
- `levelLabel(level): string`
- `PinoLogger` (`new`, `isLevelEnabled`, `bindStr`, `bindInt`, `bindBool`, `formatLine`)
