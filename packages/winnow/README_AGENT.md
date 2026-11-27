# winnow@1.0.0 (Agent Quickref)

> Zero-allocation byte-level zero-copy parser combinators (winnow / nom parity) with rollback-on-failure cursor semantics for CP. | License: MIT
> Import: `import { ... } from "winnow";` | Full types: `src/index.d.cp`

## Functions
- `export fn tag(cur: &mut Cursor, lit: string): bool;`
- `export fn tagNoCase(cur: &mut Cursor, lit: string): bool;`
- `export fn charLit(cur: &mut Cursor, ch: u8): bool;`
- `export fn take(cur: &mut Cursor, count: usize): SpanResult;`
- `export fn takeUntil(cur: &mut Cursor, needle: string): SpanResult;`
- `export fn takeTillByte(cur: &mut Cursor, stop: u8): SpanResult;`
- `export fn space0(cur: &mut Cursor): SpanResult;`
- `export fn multispace0(cur: &mut Cursor): SpanResult;`
- `export fn digit1(cur: &mut Cursor): SpanResult;`
- `export fn alpha1(cur: &mut Cursor): SpanResult;`
- `export fn alphanumeric1(cur: &mut Cursor): SpanResult;`
- `export fn hexDigit1(cur: &mut Cursor): SpanResult;`
- `export fn parseU64(cur: &mut Cursor): UintResult;`
- `export fn parseI64(cur: &mut Cursor): IntResult;`
- `export fn delimited(cur: &mut Cursor, open: u8, close: u8): SpanResult;`
