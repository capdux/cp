# @cap/zod-validation-error

Zero-heap human-readable validation error aggregator and formatter in pure CP.

## API

- `ValidationIssue { path, message, code }`
- `FormatErrorOptions { prefix, prefix_sep, issue_sep, max_issues, include_path }`
- `defaultFormatErrorOptions(): FormatErrorOptions`
- `formatValidationIssues(issues, opts, out): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`zod-validation-error@3.4.0`)
- **Upstream Repository**: https://github.com/causaly/zod-validation-error
- **License**: MIT (clean-room CP implementation)
