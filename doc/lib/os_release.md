# `cap:os_release` — freedesktop.org `/etc/os-release` Parser

Zero-allocation freedesktop.org `/etc/os-release` parser supporting single/double-quoted shell escapes, last-assignment-wins lookup, and `ID`/`ID_LIKE` distro family matching.

## Exports

- `OsReleaseField` (`decodeValue(out)`)
- `OsReleaseIter` (`init(content)`, `next(*_): OsReleaseField?`)
- `getOsReleaseField(content, key, out): ([]const u8)?`
- `isOsLike(content, distro_id): bool`
