# `cap:http_csp` — W3C Content-Security-Policy Level 3 Parser & Builder

Zero-allocation W3C CSP3 header parser, first-directive-wins lookup, `default-src` fetch fallback, nonce/keyword checks, and policy builder.

## Exports

- `CspDirective` — `hasToken`, `allowsNone`, `allowsSelf`, `allowsUnsafeInline`, `allowsUnsafeEval`, `allowsNonce`.
- `CspSourceIter` & `CspDirectiveIter` — Zero-copy streaming iterators.
- `findDirective(policy, directive_name): CspDirective?` — First occurrence wins per W3C CSP3 §2.2.1.
- `effectiveFetchDirective(policy, directive_name): CspDirective?` — Falls back to `default-src`.
- `appendCspDirective(out, pos: *usize, name, value): bool`
