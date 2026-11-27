# change-case — AI Agent Quick Ref

Zero-heap full-featured case conversion library covering all 12 change-case v5 styles (camel, pascal, snake, kebab, constant, dot, path, train, capital, sentence, no, swap). (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ccaseIsUpper,
    ccaseIsLower,
    ccaseIsDigit,
    ccaseIsAlnum,
    ccaseToUpper,
    ccaseToLower,
    ccaseJoinLower,
    ccaseCapitalizeWords,
} from "change-case";
```

## API Signatures

```cp
fn ccaseIsUpper(c: u8): bool;
fn ccaseIsLower(c: u8): bool;
fn ccaseIsDigit(c: u8): bool;
fn ccaseIsAlnum(c: u8): bool;
fn ccaseToUpper(c: u8): u8;
fn ccaseToLower(c: u8): u8;
fn ccaseJoinLower(input: string, sep: string, out: []u8): usize;
fn ccaseCapitalizeWords(out: []u8, len: usize, sep_byte: u8);
fn camelCase(input: string, out: []u8): usize;
fn pascalCase(input: string, out: []u8): usize;
fn snakeCase(input: string, out: []u8): usize;
fn kebabCase(input: string, out: []u8): usize;
fn constantCase(input: string, out: []u8): usize;
fn dotCase(input: string, out: []u8): usize;
fn pathCase(input: string, out: []u8): usize;
fn trainCase(input: string, out: []u8): usize;
fn capitalCase(input: string, out: []u8): usize;
fn sentenceCase(input: string, out: []u8): usize;
fn noCase(input: string, out: []u8): usize;
fn swapCase(input: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { ccaseJoinLower } from "change-case";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ccaseJoinLower("example", "example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"change-case"`; install via `cap add change-case`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
