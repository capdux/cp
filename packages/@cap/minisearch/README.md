# `@cap/minisearch`

Zero-heap-allocation in-memory inverted index and BM25 full-text search engine in pure CP.

## Features

- **Zero Heap Allocation**: Static pools (`MAX_DOCS = 64`, `MAX_TERMS = 256`, `MAX_POSTINGS = 512`) with FNV-1a 64-bit term hashing and linked posting lists.
- **BM25 Ranking (`k1 = 1.2, b = 0.75`)**: Multi-term IDF/TF scoring with document length normalization.
- **Prefix Autocomplete (`searchPrefix`)**: Matches all indexed terms sharing a normalized ASCII prefix.
- **Case-Insensitive ASCII Tokenization**: Splits on punctuation/whitespace and normalizes ASCII letters to lowercase.

## API

```cp
import { MAX_DOCS, MAX_TERMS, MAX_POSTINGS, SearchHit, MiniSearch } from "@cap/minisearch";
```

## License

MIT
