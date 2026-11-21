# `cap:semver_satisfies` — NPM / Bun Semver Range Matcher & Comparator

Zero-allocation, single-pass SemVer 2.0.0 version parser, comparator, and NPM/Bun range evaluator in pure CP.

## Types

### `SemverVersion`

```cp
pub struct SemverVersion {
    major: u64;
    minor: u64;
    patch: u64;
    pre: []const u8;
}
```

- `SemverVersion.parse(input: []const u8): SemverVersion?` — Parses a SemVer 2.0.0 version (optional `v`/`V` prefix, ignored `+build` metadata, strict no-leading-zero validation).
- `compare(other: SemverVersion): i32` — Compares two versions per SemVer 2.0.0 precedence (`-1`, `0`, `1`). Release versions outrank prereleases; prerelease identifiers are compared numerically when all-digits and lexicographically otherwise.

## Exports

- `satisfiesRange(version_str: []const u8, range_str: []const u8): bool` — Parses `version_str` and evaluates it against an NPM/Bun range expression in a single zero-allocation pass.
- `satisfiesVersion(ver: SemverVersion, range_str: []const u8): bool` — Evaluates a pre-parsed `SemverVersion` against a range expression.
- `isValidSemverRange(range_str: []const u8): bool` — Validates range expression syntax without heap allocation.

### Supported Range Syntax

- Logical OR (`||`) and whitespace-separated logical AND
- Comparators: `=`, `>`, `>=`, `<`, `<=` (with or without space after operator)
- Caret ranges: `^1.2.3`, `^0.2.3`, `^0.0.3`, `^1.2`, `^0.0`
- Tilde ranges: `~1.2.3`, `~1.2`, `~1`
- Wildcards / X-ranges: `*`, `x`, `X`, `1.x`, `1.2.x`
- Hyphen ranges: `1.2.3 - 2.3.4`, `1.2 - 2.3`
- Prerelease tuple guard: a prerelease version matches a comparator set only if at least one comparator in that set specifies a prerelease tag with the exact same `[major, minor, patch]` tuple.

## Example

```cp
import { SemverVersion, satisfiesRange, isValidSemverRange } from "cap:semver_satisfies";

const ok = satisfiesRange("1.5.2", "^1.2.3 || >=2.0.0 <3.0.0");
const valid = isValidSemverRange("1.2.3 - 2.3.4");
if (v := SemverVersion.parse("v1.2.3-alpha.1+build.42")) {
    // v.major == 1, v.minor == 2, v.patch == 3, v.pre == "alpha.1"
}
```
