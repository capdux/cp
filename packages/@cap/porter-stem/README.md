# `@cap/porter-stem`

Zero-heap-allocation Martin Porter (1980) English Stemmer (Steps 1a, 1b, 1c, 2, 3, 4, 5a, 5b) in pure CP.

## Features

- **`stem`**: Full Porter Stemmer producing canonical lowercase stems up to 128 bytes.
- **`measure`**: Computes the Porter consonant-vowel sequence measure `m`.
- **`sameStem`**: Fast zero-heap stem equivalence check.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { stem, measure, sameStem } from "@cap/porter-stem";
```

## License

MIT
