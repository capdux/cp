# `cap:rfc7807_problem` — RFC 9457 / RFC 7807 HTTP Problem Details

Zero-allocation RFC 9457 / RFC 7807 `application/problem+json` structured HTTP error builder and RFC 8259-escaped JSON serializer.

## Exports

- `PROBLEM_JSON_CONTENT_TYPE` (`"application/problem+json"`)
- `defaultTitleForStatus(status: u16): []const u8`
- `ProblemDetails` — `fromStatus(status, detail)`, `custom(type_uri, title, status, detail, instance)`, `toJson(out): ([]const u8)?`.
