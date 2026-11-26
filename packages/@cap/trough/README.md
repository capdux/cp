# @cap/trough

Zero-heap middleware pipeline engine and tag dispatcher in pure CP (also covers `unified` pipeline composition and `zwitch` tag routing).

## API

- `TROUGH_CAPACITY: usize = 32`
- `StageOutcome { ok: bool, value: i64, err_code: i32, halted: bool }`
- `StageFn` (`*const fn (i64) StageOutcome`)
- `stageOk(value: i64): StageOutcome` / `stageErr(err_code: i32): StageOutcome` / `stageHalt(value: i64): StageOutcome`
- `TroughPipeline`:
  - `TroughPipeline.new(): TroughPipeline`
  - `use(*_, stage: StageFn): bool`
  - `run(initial: i64): StageOutcome`
  - `len(): usize`
  - `clear(*_)`
- `TagRoute { tag: string, handler_id: i32 }`
- `dispatchTag(tag: string, routes: []const TagRoute): i32`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`trough@2.2.0`, covers `unified` & `zwitch`)
- **Upstream Repository**: https://github.com/wooorm/trough
- **License**: MIT (clean-room CP implementation)
