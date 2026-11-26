# `@cap/porter-stem`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:stemmer` ([https://github.com/words/stemmer](https://github.com/words/stemmer)) / `crates.io:rust-stemmers` ([https://github.com/CurrySoftware/rust-stemmers](https://github.com/CurrySoftware/rust-stemmers)) / `crates.io:porter-stemmer` ([https://github.com/jwilm/porter-stemmer](https://github.com/jwilm/porter-stemmer))（原库名 `stemmer` → CP 包名 `@cap/porter-stem`）

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
