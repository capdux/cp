# `cap:byte_size` — Human-Readable Byte-Size Parser & SI/IEC Formatter

Zero-allocation pure-integer parser for SI (`KB`/`MB`/`GB`/`TB`/`PB`) and IEC (`KiB`/`MiB`/`GiB`/`TiB`/`PiB`) byte sizes with fractional support and canonical formatters.

## Exports

- `parseByteSize(input: []const u8): u64?` — Parses `"512"`, `"10KB"`, `"1.5 GiB"`, `"2.25MB"` into exact `u64` bytes with overflow protection.
- `formatBytesIec(bytes: u64, out: []u8): ([]const u8)?` — Formats with 1024-based IEC units (`B`, `KiB`, `MiB`, `GiB`, `TiB`, `PiB`).
- `formatBytesSi(bytes: u64, out: []u8): ([]const u8)?` — Formats with 1000-based SI units (`B`, `KB`, `MB`, `GB`, `TB`, `PB`).
