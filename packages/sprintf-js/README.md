# @cap/sprintf-js

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:sprintf-js` ([https://github.com/alexei/sprintf.js](https://github.com/alexei/sprintf.js), `BSD-3-Clause`)

Zero-heap-allocation C-style `sprintf` formatting engine in pure CP, modeled after `sprintf-js`.

## Features

- `FmtArgKind`: `Int`, `Uint`, `Float`, `Str`, `Char`, `Bool`
- `FmtArg`: tagged value with static constructors `.int(i64)`, `.uint(u64)`, `.float(f64)`, `.str(string)`, `.ch(u8)`, `.boolean(bool)`
- `sprintf(fmt: string, args: []const FmtArg, out: []u8): usize`:
  - Specifiers: `%%`, `%s`, `%d`, `%i`, `%u`, `%x`, `%X`, `%o`, `%b`, `%f`, `%c`
  - Flags: `-` (left-align), `+` (explicit `+` sign), `0` (zero-padding)
  - Minimum field width (`N`) and precision (`.N` for `%f` decimals and `%s` max byte length)

## License

BSD-3-Clause
