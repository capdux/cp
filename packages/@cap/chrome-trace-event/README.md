# @cap/chrome-trace-event

Zero-heap Chrome DevTools / Perfetto Trace Event JSON format serializer in pure CP.

## API

- `TracePhase` enum (`Begin`, `End`, `Complete`, `Instant`, `Counter`, `Metadata`)
- `TraceEvent { name, cat, ph, ts_us, dur_us, pid, tid }`
- `phaseCode(ph: TracePhase): u8` (`'B'`, `'E'`, `'X'`, `'i'`, `'C'`, `'M'`)
- `formatTraceEvent(ev: TraceEvent, out: []u8): usize`
- `formatTraceArray(events: []const TraceEvent, out: []u8): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`chrome-trace-event@1.0.4`)
- **Upstream Repository**: https://github.com/samccone/chrome-trace-event
- **License**: MIT (clean-room CP implementation)
