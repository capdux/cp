# `@cap/logfmt`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:logfmt` ([https://github.com/csquared/node-logfmt](https://github.com/csquared/node-logfmt))

Zero-heap-allocation `logfmt` structured key=value log parser and writer in pure CP.

## Features

- **`LogfmtRecord.parse`**: Parses `k=v`, quoted escaped strings (`\"`, `\\`, `\n`, `\r`, `\t`), bare boolean flags (`debug` -> `"true"`), and empty values (`err=`) with last-write-wins lookup (`get` / `has`).
- **`LogfmtWriter`**: Atomic zero-heap serialization for strings (auto-quoting/escaping), signed integers (`writeInt`), and booleans (`writeBool`).
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI (`MAX_FIELDS = 64`).

## API

```cp
import { MAX_FIELDS, LogfmtField, LogfmtRecord, LogfmtWriter } from "@cap/logfmt";
```

## License

MIT
