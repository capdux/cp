# `cap:etc_shells` — Linux `/etc/shells` Zero-Allocation Parser & Validator

Zero-allocation, pure-CP streaming parser and validator for Linux `/etc/shells` valid login shell lists.

## Exports

### Types

- `EtcShellsIter`
  - `EtcShellsIter.init(content: []const u8): EtcShellsIter`
  - `next(*_): ([]const u8)?` — Streams trimmed absolute shell paths across `\n` and `\r\n` lines, skipping blank lines, `#` comments, non-absolute paths, directory paths ending with `/`, and malformed lines containing whitespace or control bytes.

### Functions

- `parseShellLine(raw: []const u8): ([]const u8)?` — Validates and extracts a single `/etc/shells` line.
- `isAllowedShell(content: []const u8, shell_path: []const u8): bool` — Exact absolute path membership check.
- `findShellByBasename(content: []const u8, basename: []const u8): ([]const u8)?` — Finds the first shell path whose basename matches `basename` (e.g., `"bash"` -> `"/bin/bash"`).
- `countShells(content: []const u8): usize` — Counts valid shell entries.
- `isNologinShell(shell_path: []const u8): bool` — Identifies `/sbin/nologin`, `/usr/sbin/nologin`, `/bin/false`, and `/usr/bin/false`.

## Example

```typescript
import {
    EtcShellsIter,
    isAllowedShell,
    findShellByBasename,
    countShells,
    isNologinShell,
} from "cap:etc_shells";

const content =
    "# /etc/shells: valid login shells\n" ++
    "/bin/sh\n" ++
    "/bin/bash\n" ++
    "/usr/bin/zsh\n";

const ok = isAllowedShell(content, "/bin/bash");       // true
const zsh = findShellByBasename(content, "zsh");       // "/usr/bin/zsh"
const n = countShells(content);                        // 3
const blocked = isNologinShell("/usr/sbin/nologin");   // true
```
