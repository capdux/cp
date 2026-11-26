# @cap/syslog

Zero-heap RFC 5424 Syslog message parser and formatter in pure CP.

## Features

- Standard severity (`SEV_EMERG`..`SEV_DEBUG`) and facility (`FAC_KERN`, `FAC_USER`, `FAC_DAEMON`, `FAC_AUTH`, `FAC_LOCAL0`) constants
- `encodePri`, `decodeFacility`, `decodeSeverity`
- Zero-copy `parseRfc5424(raw)` handling `<PRI>1 TIMESTAMP HOSTNAME APP-NAME PROCID MSGID SD MSG`, NILVALUE `-`, and structured data blocks `[id k="v"]`
- `formatRfc5424(...)` with automatic `-` NILVALUE emission and buffer overflow protection

## Usage

```cp
import { formatRfc5424, parseRfc5424, FAC_LOCAL0, SEV_INFO } from "@cap/syslog";

let buf: [256]u8 = undefined;
const n = formatRfc5424(FAC_LOCAL0, SEV_INFO, "2026-11-26T12:00:00Z", "node1", "capd", "42", "ID1", "started", buf[0..256]);
const parsed = parseRfc5424(buf[0..(n as usize)]);
```
