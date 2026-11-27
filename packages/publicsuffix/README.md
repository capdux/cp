# `@cap/publicsuffix`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:publicsuffix` ([https://github.com/rushmorem/publicsuffix](https://github.com/rushmorem/publicsuffix))

Zero-heap pure-CP Public Suffix List (PSL) domain suffix (eTLD) and registrable domain (eTLD+1) parser with custom wildcard (`*.ck`) and exception (`!www.ck`) rules — clean-room CP equivalent of crates.io `publicsuffix`.

## Features

- Built-in single-level TLDs (`.com`, `.org`, `.net`, `.io`, `.dev`, `.ai`, `.cn`, `.jp`, `.uk`, `.de`, `.fr`, `.au`, `.app`, `.xyz`, etc.) and multi-level public suffixes (`.co.uk`, `.com.cn`, `.co.jp`, `.com.au`, `.github.io`, `.pages.dev`, `.workers.dev`, `.amazonaws.com`, `.s3.amazonaws.com`, etc.)
- `PslRuleList`: fixed-capacity zero-heap custom rule table supporting exact, wildcard (`*.tld`), and exception (`!host.tld`) rules with PSL precedence (exception > longest match > default single-label TLD)
- `parseDomain(host)`, `suffix(host)`, `registrableDomain(host)`, `isPublicSuffix(host)`
- Case-insensitive normalization, trailing FQDN `.` stripping, and strict hostname validation
