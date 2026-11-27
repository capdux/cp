# ical-generator — AI Agent Quick Ref

Zero-allocation RFC 5545 iCalendar (`.ics`) `VCALENDAR`/`VEVENT` generator and zero-copy parser in pure CP (npm `ical-generator` parity).

## Import

```cp
import {
    IcalEvent,
    ParsedEvent,
    IcalWriter,
    parseFirstEvent,
    unescapeText,
} from "ical-generator";
```

## API Signatures

```cp
class IcalEvent {
    uid: &[]u8, dtstart: &[]u8, dtend: &[]u8,
    summary: &[]u8, location: &[]u8, description: &[]u8,
}

class ParsedEvent {
    uid_start: usize, uid_len: usize,
    dtstart_start: usize, dtstart_len: usize,
    dtend_start: usize, dtend_len: usize,
    summary_start: usize, summary_len: usize,
    location_start: usize, location_len: usize,
    valid: bool,
}

class IcalWriter {
    buf: []u8, pos: usize, event_count: usize, failed: bool, finished: bool,
    static fn init(buf: []u8, prod_id: &[]u8): IcalWriter;
    fn addEvent(*self, ev: IcalEvent): bool;
    fn finish(*self): i64;
    fn eventCount(): usize;
    fn written(): &[]u8;
}

fn parseFirstEvent(ics: &[]u8): ParsedEvent;
fn unescapeText(src: &[]u8, dst: []u8): i64;
```

## Minimal Example

```cp
import { IcalWriter, IcalEvent, parseFirstEvent } from "ical-generator";

let buf: [1024]u8 = undefined;
let w = IcalWriter.init(buf[0..1024], "-//CP//App//EN");
let _ = w.addEvent(IcalEvent {
    uid: "1@cp", dtstart: "20261127T090000Z", dtend: "20261127T100000Z",
    summary: "Sync", location: "", description: "",
});
let len = w.finish();
if (len > 0) {
    let ev = parseFirstEvent(w.written());
}
```

## Pitfalls & Borrowing Rules

- **Negative `i64` error sentinel**: `finish()` and `unescapeText()` return `-1` on buffer overflow or invalid state; always check `>= 0` before casting to `usize`.
- **Call `finish()` once**: After `finish()`, further `addEvent()` calls return `false`.
- **Buffer lifetime**: `IcalWriter` borrows `buf: []u8` mutably and `written()` returns a read-only view into `buf`; keep `buf` alive while using `w`.
- **Span extraction**: `ParsedEvent` stores byte offsets (`*_start`, `*_len`) into the original `ics` slice; slice `ics[start .. start + len]` and call `unescapeText` if needed.
