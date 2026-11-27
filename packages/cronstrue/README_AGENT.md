# cronstrue@1.0.0 — AI Agent Quick Ref

> Zero-alloc Cron to English/Chinese natural language translator | Source: `npm:cronstrue` | License: MIT
> Import: `import { Locale, CronDescOptions, describeCron, describeCronWith, isValidCron } from "cronstrue";` | Types: `src/index.d.cp`

## Core Types

```cp
export enum Locale { En = 0, Zh = 1 }

export class CronDescOptions {
    locale: Locale,
    use_24h: bool,
    fn english(): CronDescOptions;
    fn chinese(): CronDescOptions;
}

export class ParsedCron {
    valid: bool,
    minute: CronField, hour: CronField, dom: CronField, month: CronField, dow: CronField,
}
```

## Core Functions

- `isValidCron(expr: string): bool` — Validate 5-field POSIX Cron or macro.
- `describeCron(expr: string, out: []u8): string` — English 24h description; returns `""` if invalid.
- `describeCronWith(expr: string, opts: CronDescOptions, out: []u8): string` — Custom locale (`En`/`Zh`) & `use_24h`.
- `parseCron(raw: string): ParsedCron` — Structured parse into `CronField` (`items: [16]FieldItem`).
- `expandMacro(trimmed: string): string` — Expand `@yearly`/`@annually`/`@monthly`/`@weekly`/`@daily`/`@midnight`/`@hourly`.
- `dayName(val: u8, locale: Locale): string` | `monthName(val: u8, locale: Locale): string`

## Minimal Usage

```cp
import { CronDescOptions, describeCron, describeCronWith } from "cronstrue";

let buf: [128]u8 = undefined;
const en = describeCron("0 9 * * 1-5", buf[0..128]);
const zh = describeCronWith("0 9 * * 1-5", CronDescOptions.chinese(), buf[0..128]);
```

## Pitfalls & Rules

1. **Buffer lifetime**: Returned `string` slices `out`; do not overwrite `out` while still using the previous result.
2. **Invalid input**: `describeCron` / `describeCronWith` return `""` (empty string) on invalid expressions — check `res.len > 0` or pre-validate with `isValidCron`.
3. **List item cap**: Each field holds up to 16 comma-separated items (`[16]FieldItem`); more than 16 items fails validation.
4. **DOW 0 and 7**: Both `0` and `7` map to Sunday; named `JAN`–`DEC` and `SUN`–`SAT` are case-insensitive.
5. **5-field POSIX only**: Standard 5 fields + `@` macros; 6-field (seconds) syntax is rejected.
