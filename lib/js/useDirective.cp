// Source: Absorbed from /data/dev/bun/src/js_parser/ast/UseDirective.cp | Synced: 2026-11-22T15:28:25Z
// Zero-allocation JS/RSC directive prologue parser (`"use client"`, `"use server"`, `"use strict"`)
// and module boundary transition checker.

/// Target boundary kind when crossing between Server and Client component graphs.
pub enum UseDirectiveBoundering: u8 {
    Client = 1,
    Server = 2,
}

/// Aggregated directive flags collected from a module's directive prologue or module graph.
pub struct UseDirectiveFlags {
    has_any_client: bool = false,
    has_any_server: bool = false,
    has_use_strict: bool = false,

    fn record(*_, dir: UseDirective) {
        switch (dir) {
            .Client => _.has_any_client = true,
            .Server => _.has_any_server = true,
            .Strict => _.has_use_strict = true,
            .None => {},
        }
    }

    fn hasAnyBoundary(): bool {
        return _.has_any_client || _.has_any_server;
    }
}

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn isSpaceOrSemi(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ';';
}

/// Skip leading UTF-8 BOM, whitespace, empty statements (`;`), and `//` / `/* ... */` comments.
fn skipLeadingTrivia(s: []const u8, start: usize): usize {
    let i = start;
    if (i == 0 && s.len >= 3 && s[0] == 0xEF && s[1] == 0xBB && s[2] == 0xBF) {
        i = 3;
    }
    while (i < s.len) {
        if (isSpaceOrSemi(s[i])) {
            i += 1;
            continue;
        }
        if (i + 1 < s.len && s[i] == '/' && s[i + 1] == '/') {
            i += 2;
            while (i < s.len && s[i] != '\n' && s[i] != '\r') {
                i += 1;
            }
            continue;
        }
        if (i + 1 < s.len && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            let closed = false;
            while (i + 1 < s.len) {
                if (s[i] == '*' && s[i + 1] == '/') {
                    i += 2;
                    closed = true;
                    break;
                }
                i += 1;
            }
            if (!closed) return s.len;
            continue;
        }
        break;
    }
    return i;
}

fn isDirectiveTerminator(s: []const u8, idx: usize): bool {
    if (idx >= s.len) return true;
    const c = s[idx];
    if (c == ';' || c == '\n' || c == '\r' || c == ' ' || c == '\t') return true;
    if (c == '/' && idx + 1 < s.len && (s[idx + 1] == '/' || s[idx + 1] == '*')) return true;
    return false;
}

/// JS/TS/RSC module top-level directive (absorbed from Bun `js_ast.UseDirective`).
pub enum UseDirective: u8 {
    None = 0,
    /// `"use client"`
    Client = 1,
    /// `"use server"`
    Server = 2,
    /// `"use strict"`
    Strict = 3,

    /// True if importing a module with `other` directive crosses an RSC boundary.
    fn isBoundary(other: UseDirective): bool {
        if (other != .Client && other != .Server) return false;
        if (_ == other) return false;
        return true;
    }

    /// Return the target `UseDirectiveBoundering` if transitioning from `self` to `other`
    /// crosses a Client/Server boundary, or `null` otherwise.
    fn boundering(other: UseDirective): UseDirectiveBoundering? {
        if (!_.isBoundary(other)) return null;
        return switch (other) {
            .Client => .Client,
            .Server => .Server,
            else => null,
        };
    }

    /// Canonical unquoted directive string (empty for `.None`).
    fn asLiteral(): []const u8 {
        return switch (_) {
            .Client => "use client",
            .Server => "use server",
            .Strict => "use strict",
            .None => "",
        };
    }

    /// Canonical bundler `--banner` directive string (e.g. `"\"use client\";"`).
    fn asBanner(): []const u8 {
        return switch (_) {
            .Client => "\"use client\";",
            .Server => "\"use server\";",
            .Strict => "\"use strict\";",
            .None => "",
        };
    }

    /// Parse the leading directive from source `contents`.
    /// - Returns `.None` if no quoted directive literal starts the module.
    /// - Returns `.Client` / `.Server` / `.Strict` for recognized directives.
    /// - Returns `null` for an unrecognized quoted string directive (parity with Bun `UseDirective.parse`).
    static fn parse(contents: []const u8): UseDirective? {
        const start = skipLeadingTrivia(contents, 0);
        if (start >= contents.len) return .None;

        const quote = contents[start];
        if (quote != '"' && quote != '\'' && quote != '`') {
            return .None;
        }

        let end = start + 1;
        while (end < contents.len && contents[end] != quote) {
            if (contents[end] == '\n' || contents[end] == '\r' || contents[end] == '\\') {
                return .None;
            }
            end += 1;
        }
        if (end >= contents.len) return .None;
        if (!isDirectiveTerminator(contents, end + 1)) return .None;

        const inner = contents[(start + 1)..end];
        if (strEql(inner, "use client")) return .Client;
        if (strEql(inner, "use server")) return .Server;
        if (strEql(inner, "use strict")) return .Strict;
        return null;
    }
}

/// Scan the entire leading directive prologue of `contents` (e.g. `"use strict";\n"use client";`)
/// and aggregate all recognized directives into `UseDirectiveFlags`.
pub fn scanUseDirectives(contents: []const u8): UseDirectiveFlags {
    let flags = UseDirectiveFlags{};
    let pos: usize = 0;

    while (pos < contents.len) {
        const start = skipLeadingTrivia(contents, pos);
        if (start >= contents.len) break;

        const quote = contents[start];
        if (quote != '"' && quote != '\'' && quote != '`') {
            break;
        }

        let end = start + 1;
        let malformed = false;
        while (end < contents.len && contents[end] != quote) {
            if (contents[end] == '\n' || contents[end] == '\r' || contents[end] == '\\') {
                malformed = true;
                break;
            }
            end += 1;
        }
        if (malformed || end >= contents.len) break;
        if (!isDirectiveTerminator(contents, end + 1)) break;

        const inner = contents[(start + 1)..end];
        if (strEql(inner, "use client")) {
            flags.record(.Client);
        } else if (strEql(inner, "use server")) {
            flags.record(.Server);
        } else if (strEql(inner, "use strict")) {
            flags.record(.Strict);
        }

        pos = end + 1;
        if (pos < contents.len && contents[pos] == ';') {
            pos += 1;
        }
    }

    return flags;
}
