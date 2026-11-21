# `cap:slugify` — URL-Safe Slug Generator & Validator

Zero-allocation URL slug generator with punctuation collapsing, apostrophe elision, word-boundary bounded truncation, and canonical slug validation.

## Exports

- `slugify(input, out): ([]const u8)?` — Standard `'-'` hyphenated lowercase ASCII slug.
- `slugifyWithSep(input, sep, out): ([]const u8)?` — Custom separator slug.
- `slugifyBounded(input, max_len, out): ([]const u8)?` — Word-boundary-aware bounded slug.
- `isValidSlug(s): bool` — Validates `[a-z0-9]+(-[a-z0-9]+)*`.
