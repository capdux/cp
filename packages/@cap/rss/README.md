# `@cap/rss`

Zero-heap-allocation RSS 2.0 XML feed generator and CDATA-aware tag extractor in pure CP.

## Features

- **`RssWriter`**: Generates valid RSS 2.0 XML (`<channel>` / `<item>`) with strict XML entity escaping (`& < > " '`) and automatic omission of empty optional fields.
- **`extractTagValue`**: Zero-copy extraction of `<tag>...</tag>` contents with automatic `<![CDATA[...]]>` unwrapping.
- **`countItems`**: Counts `<item>` entries in zero memory.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { RssChannel, RssItem, RssWriter, extractTagValue, countItems } from "@cap/rss";
```

## License

MIT
