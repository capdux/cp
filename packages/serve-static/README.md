# @cap/serve-static

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:serve-static` ([https://github.com/expressjs/serve-static](https://github.com/expressjs/serve-static))

Clean-room MIT, zero-heap HTTP static file request resolver, dotfile policy evaluator, and `Cache-Control`/ETag/Content-Type builder for CP — modeled on npm's `serve-static` v2.

## Dependencies
- `@cap/parseurl` (`1.0.0`) — fast request URL splitting and path traversal defense
- `@cap/mime-types` (`1.0.0`) — MIME & Content-Type resolution
- `@cap/etag` (`1.0.0`) — weak stat-based ETag generation
- `@cap/itoa` (`1.0.0`) — zero-heap `max-age` formatting

## API
- `ServeStaticOptions` (`defaultOpts`) & dotfile modes (`DOTFILE_IGNORE`, `DOTFILE_DENY`, `DOTFILE_ALLOW`)
- `resolveStaticRequest(method, raw_url, opts, path_out): StaticResolution`
- `hasDotfileSegment(pathname): bool`
- `formatCacheControl(max_age_secs, immutable, out): usize`
- `formatStaticEtag(size_bytes, mtime_ms, out): usize`
- `formatStaticContentType(file_path, out): usize`
