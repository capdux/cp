# `@cap/json-pointer`

Zero-heap-allocation RFC 6901 JSON Pointer implementation in pure CP.

## Features

- **Validation (`isValid`)**: Enforces RFC 6901 leading `'/'` and strict `~0`/`~1` escaping.
- **Escaping (`escapeToken` / `unescapeToken`)**: Exact RFC 6901 `~` <-> `~0`, `/` <-> `~1` conversion without double-unescaping (`~01` -> `~1`).
- **Navigation (`countSegments`, `hasSegment`, `getSegment`, `parentPointer`, `appendSegment`)**: Zero-copy slicing and caller-buffer segment extraction.
- **Strict Array Index (`parseArrayIndex`)**: Supports `"0"`, non-leading-zero indices, and `"-"` (`-2`).
- **Streaming Iterator (`PointerIter`)**: Zero-allocation forward traversal over unescaped tokens.

## API

```cp
import {
    isValid,
    escapeToken,
    unescapeToken,
    countSegments,
    hasSegment,
    getSegment,
    parseArrayIndex,
    appendSegment,
    parentPointer,
    PointerIter,
} from "@cap/json-pointer";
```

## License

MIT
