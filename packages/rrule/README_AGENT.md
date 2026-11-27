# rrule@1.0.0 (Agent Quickref)

> Zero-allocation RFC 5545 iCalendar RRULE parser, validator & formatter (npm rrule v2 parity) composing @cap/unicase for CP | License: MIT
> Import: `import { ... } from "rrule";` | Full types: `src/index.d.cp`
> Deps: `unicase`

## Functions
- `export fn parseRRule(input: string): RRule;`
- `export fn formatRRule(r: RRule, out: []u8): usize;`
- `export fn matchesWeekday(r: RRule, dow: u8): bool;`
- `export fn isValidRRule(input: string): bool;`
