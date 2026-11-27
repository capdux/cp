# send@1.0.0 (Agent Quickref)

> Zero-heap pure CP static file transfer planner with traversal protection, dotfiles policy, ETag/304, Range/206, and redirect URL encoding (clean-room MIT implementation of npm send v1) | License: MIT
> Import: `import { ... } from "send";` | Full types: `src/index.d.cp`
> Deps: `encodeurl`, `etag`, `mime-types`, `range-parser`

## Functions
- `export fn planSendFile(req_path: string, file_size: u64, mtime_ms: u64, range_header: string, if_none_match: string, opts: SendOptions, etag_out: []u8): SendPlan;`
- `export fn formatRedirectUrl(path: string, out: []u8): usize;`
