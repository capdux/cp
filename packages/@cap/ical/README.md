# `@cap/ical`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ical-generator` ([https://github.com/sebbo2002/ical-generator](https://github.com/sebbo2002/ical-generator))（原库名 `ical-generator` → CP 包名 `@cap/ical`）

Zero-heap-allocation RFC 5545 iCalendar (`.ics`) generator and parser in pure CP.

## Features

- **`IcalWriter`**: Generates RFC 5545 `VCALENDAR` / `VEVENT` streams with CRLF (`\r\n`) line endings and automatic TEXT escaping (`\n`, `\,`, `\;`, `\\`).
- **`parseFirstEvent`**: Zero-copy extraction of `UID`, `DTSTART`, `DTEND`, `SUMMARY`, and `LOCATION` offsets, handling property parameters such as `DTSTART;TZID=UTC:...`.
- **`unescapeText`**: Restores RFC 5545 escaped sequences (`\n`/`\N`, `\,`, `\;`, `\\`).
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { IcalEvent, IcalWriter, ParsedEvent, parseFirstEvent, unescapeText } from "@cap/ical";
```

## License

MIT
