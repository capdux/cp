[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/file-entry-cache

Zero-heap incremental build and lint file metadata cache in pure CP (also covers `flat-cache`). Tracks file paths, modification timestamps, byte sizes, content hashes, and cached payloads in a fixed 64-slot stack table.

## API

- `FileCacheEntry { path: string, mtime_ms: i64, size: u64, content_hash: u64, payload: string, valid: bool }`
- `FileEntryCache` (fixed 64-slot capacity):
  - `FileEntryCache.new(): FileEntryCache`
  - `set(*_, path, mtime_ms, size, content_hash, payload): bool`
  - `get(path): FileCacheEntry?`
  - `hasChanged(path, mtime_ms, size, content_hash): bool`
  - `remove(*_, path): bool`
  - `reconcile(*_, existing_paths: []const string): usize`
  - `len(): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`file-entry-cache@10.0.0`, covers `flat-cache`)
- **Upstream Repository**: https://github.com/jaredwray/cacheable
- **License**: MIT (clean-room CP implementation)
