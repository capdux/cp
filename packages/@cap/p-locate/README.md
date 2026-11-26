# @cap/p-locate

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:p-locate` ([https://github.com/sindresorhus/p-locate](https://github.com/sindresorhus/p-locate))

Zero-allocation, Clean-room MIT implementation of `p-locate` v7 bounded-concurrency locator planning for CP, reusing `@cap/p-limit-core`.

## Features

- **`PLocateOptions`**: `{ concurrency: usize, preserve_order: bool }` with `PLocateOptions.defaults()` (`concurrency = 4`, `preserve_order = true`).
- **`LocateCandidate`**: `{ id: u32, matched: bool, finished: bool }`.
- **`PLocatePlanner`**: Zero-heap 32-slot locator scheduler backed by `PLimit`:
  - `PLocatePlanner.new(opts: PLocateOptions): PLocatePlanner?` (rejects `concurrency == 0`)
  - `addCandidate(*_, id: u32): bool`
  - `dispatchNext(*_): u32?`
  - `reportResult(*_, id: u32, matched: bool): u32?`
  - `isSettled(): bool` / `winnerId(): u32?`

## Dependencies

- `@cap/p-limit-core` (`1.0.0`)

## License

MIT
