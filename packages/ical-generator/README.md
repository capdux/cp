[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ical-generator

Zero-allocation RFC 5545 iCalendar (`.ics`) `VCALENDAR` / `VEVENT` generator, text escaper, and lightweight event parser in pure CP.

Aligned with npm [`ical-generator`](https://github.com/sebbo2002/ical-generator) (MIT).

## Installation & Import

```bash
cap add ical-generator
```

```cp
import {
    IcalEvent,
    ParsedEvent,
    IcalWriter,
    parseFirstEvent,
    unescapeText,
} from "ical-generator";
```

## Core API

### Data Structures

| Class | Fields | Description |
| --- | --- | --- |
| `IcalEvent` | `uid`, `dtstart`, `dtend`, `summary`, `location`, `description` (all `&[]u8` / `[]const u8`) | Input event descriptor; empty `location` / `description` fields are omitted when serialized. |
| `ParsedEvent` | `uid_start/len`, `dtstart_start/len`, `dtend_start/len`, `summary_start/len`, `location_start/len: usize`, `valid: bool` | Zero-copy byte-span offsets into the original `.ics` buffer for the first `VEVENT`. |

### `IcalWriter` Class

| Member | Signature | Description |
| --- | --- | --- |
| `IcalWriter.init` | `(buf: []u8, prod_id: &[]u8): IcalWriter` | Initializes writer into caller buffer `buf` and writes `BEGIN:VCALENDAR`, `VERSION:2.0`, `PRODID`, `CALSCALE:GREGORIAN`. |
| `addEvent` | `(*self, ev: IcalEvent): bool` | Appends a `VEVENT` block with RFC 5545 text escaping (`SUMMARY`, `LOCATION`, `DESCRIPTION`); returns `false` on overflow or after `finish()`. |
| `finish` | `(*self): i64` | Appends `END:VCALENDAR\r\n` and returns total bytes written, or `-1` on failure/overflow. |
| `eventCount` | `(): usize` | Returns number of events successfully added. |
| `written` | `(): &[]u8` | Returns immutable slice of all `.ics` bytes written so far. |

### Parser & Unescape Functions

| Function | Signature | Description |
| --- | --- | --- |
| `parseFirstEvent` | `(ics: &[]u8): ParsedEvent` | Parses the first `VEVENT` block in `ics` into zero-copy byte spans (`valid == true` if found). |
| `unescapeText` | `(src: &[]u8, dst: []u8): i64` | Unescapes RFC 5545 sequences (`\n`, `\,`, `\;`, `\\`) into `dst`; returns written bytes or `-1` on overflow. |

## Examples

### 1. Generating an RFC 5545 `.ics` Calendar

```cp
import { print } from "cap:io";
import { IcalWriter, IcalEvent } from "ical-generator";

pub fn main() {
    let buf: [2048]u8 = undefined;
    let w = IcalWriter.init(buf[0..2048], "-//CP//Calendar 1.0//EN");
    let ev = IcalEvent {
        uid: "evt-001@cp-lang.dev",
        dtstart: "20261127T090000Z",
        dtend: "20261127T100000Z",
        summary: "CP Release Sync, Team",
        location: "Room A; Floor 3",
        description: "Agenda:\n1. Compiler\n2. Stdlib",
    };
    let _ = w.addEvent(ev);
    let total = w.finish();
    if (total > 0) {
        print("{}\n", w.written());
    }
}
```

### 2. Parsing & Unescaping a `VEVENT`

```cp
import { print } from "cap:io";
import { parseFirstEvent, unescapeText } from "ical-generator";

pub fn inspect(ics: []const u8) {
    let ev = parseFirstEvent(ics);
    if (ev.valid) {
        let raw_summary = ics[ev.summary_start .. ev.summary_start + ev.summary_len];
        let decoded: [256]u8 = undefined;
        let n = unescapeText(raw_summary, decoded[0..256]);
        if (n >= 0) {
            print("summary: {}\n", decoded[0..n]);
        }
    }
}
```

## Differences from Upstream & Zero-Heap Highlights

- **Zero heap allocation**: `IcalWriter` streams CRLF-formatted RFC 5545 output directly into a caller-provided `[]u8` buffer with overflow tracking (`failed` flag / `-1` return).
- **Zero-copy span parsing**: `parseFirstEvent` returns index/length spans into the source `.ics` slice rather than allocating field strings.
- **RFC 5545 escaping built-in**: Automatically escapes/unescapes `\`, `,`, `;`, and `\n` in text properties without regex or temporary buffers.
