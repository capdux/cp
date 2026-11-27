# negotiator — AI Agent Quick Ref

- **Import**: `import { NEG_MAX_SPECS, SPEC_WILDCARD, SPEC_PARTIAL, SPEC_EXACT, NegSpec, QualitySpec, parseQValue, parseWeightedList, parseAcceptList, negotiateEncoding, negotiateLanguage, negotiateCharset, bestMediaType, bestEncoding, bestLanguage } from "negotiator";`
- **Source**: npm `negotiator` (`v1.0.0`, zero-heap RFC 7231/9110 content negotiation)

## Constants
```cp
NEG_MAX_SPECS: usize = 32
SPEC_WILDCARD: u8 = 1
SPEC_PARTIAL: u8 = 2
SPEC_EXACT: u8 = 4
```

## Core Signatures
```cp
class QualitySpec { value: string, q_milli: u32, order: usize }
class NegSpec { start: usize, len: usize, q_milli: u16, specificity: u8, order: usize }

// High-level negotiation (returns matched candidate string or null)
negotiateEncoding(accept_encoding: string, available: &[]string): string?;
negotiateLanguage(accept_language: string, available: &[]string): string?;
negotiateCharset(accept_charset: string, available: &[]string): string?;
negNegotiateToken(header: string, available: &[]string, is_lang: bool): string?;

// Index-returning core negotiation (returns index in `available`, or -1)
bestMediaType(header: string, available: []const string): i64;
bestEncoding(header: string, available: []const string): i64;
bestLanguage(header: string, available: []const string): i64;

// Header parsing
parseQValue(raw: string): u32;                                  // 0..1000
parseWeightedList(header: string, out: []QualitySpec): usize;
parseAcceptList(header: string, out: []NegSpec): usize;
negTokenSpecificity(spec: string, candidate: string, is_lang: bool): u32;
negTrim(s: string): string;
negIsWs(b: u8): bool;
```

## Pitfalls
- `q=0` explicitly forbids a candidate even if a wildcard `*` is also present.
- `q_milli` uses integer thousandths (`0..1000`) to avoid floating-point rounding drift.
- `bestMediaType` / `bestEncoding` / `bestLanguage` cap header items at `NEG_MAX_SPECS` (`32`) and return `-1` when no candidate is acceptable.
- Empty `Accept` / `Accept-Language` headers default to the first candidate (`index 0`); empty `Accept-Encoding` prefers `"identity"` if present.
- Import directly from `"negotiator"` (never `negotiator`).
