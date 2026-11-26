// packages/@cap/mime-guess/src/index.cp
// Zero-allocation file path & extension MIME type guesser inspired by Rust `mime_guess` v2.
// Clean-room MIT implementation reusing `@cap/mime-types` as the underlying MIME database.

import { lookup, extension, charset, isCompressible } from "../../mime-types/src/index.cp";

// ---------------------------------------------------------------------------
// Private Helpers (prefixed with mgs)
// ---------------------------------------------------------------------------

fn mgsStartsWith(s: string, prefix: string): bool {
    if (s.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn mgsHasPathSep(s: string): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ('/' as u8) || s[i] == ('\\' as u8)) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn mgsIsCompressibleMime(mime_type: string): bool {
    return isCompressible(mime_type);
}

// ---------------------------------------------------------------------------
// Public Path/Extension & MIME Helpers
// ---------------------------------------------------------------------------

/// Extracts the pure file extension (without leading `.`) from a POSIX or Windows path.
/// Handles `/a.b/file` -> `null`, `.gitignore`/`.env` -> `null`, `archive.tar.gz` -> `"gz"`.
pub fn extractExt(path: string): string? {
    if (path.len == 0) {
        return null;
    }
    // Locate basename after last '/' or '\'
    let base_start: usize = 0;
    let i: usize = path.len;
    while (i > 0) {
        i -= 1;
        const b = path[i];
        if (b == ('/' as u8) || b == ('\\' as u8)) {
            base_start = i + 1;
            break;
        }
    }
    if (base_start >= path.len) {
        return null;
    }

    const base: string = path[base_start..path.len];
    // Locate last '.' in basename
    let j: usize = base.len;
    while (j > 0) {
        j -= 1;
        if (base[j] == ('.' as u8)) {
            // Hidden file without extension (e.g. ".gitignore") or trailing dot ("file.")
            if (j == 0 || j + 1 >= base.len) {
                return null;
            }
            return base[(j + 1)..base.len];
        }
    }
    return null;
}

/// Returns the preferred file extension for a MIME type (via `@cap/mime-types`).
pub fn getExtForMime(mime_type: string): string? {
    return extension(mime_type);
}

/// Returns the default charset for a MIME type (via `@cap/mime-types`).
pub fn getCharsetForMime(mime_type: string): string? {
    return charset(mime_type);
}

// ---------------------------------------------------------------------------
// Public MimeGuess Struct
// ---------------------------------------------------------------------------

/// Zero-allocation MIME guess result modeled after Rust's `mime_guess::MimeGuess`.
pub struct MimeGuess {
    pub mime: string,
    pub found: bool,

    /// Guesses MIME type from a file extension (with or without leading `.`, case-insensitive).
    pub static fn fromExt(ext: string): MimeGuess {
        if (ext.len == 0) {
            return MimeGuess{ mime: "", found: false };
        }
        const clean: string = if (ext[0] == ('.' as u8)) { ext[1..ext.len] } else { ext };
        if (clean.len == 0 || mgsHasPathSep(clean)) {
            return MimeGuess{ mime: "", found: false };
        }
        const res = lookup(clean);
        if (res == null) {
            return MimeGuess{ mime: "", found: false };
        }
        return MimeGuess{ mime: res.?, found: true };
    }

    /// Guesses MIME type from a POSIX or Windows file path.
    pub static fn fromPath(path: string): MimeGuess {
        const ext = extractExt(path);
        if (ext == null) {
            return MimeGuess{ mime: "", found: false };
        }
        return MimeGuess.fromExt(ext.?);
    }

    /// Returns the guessed MIME string, or `null` if unknown.
    pub fn first(): string? {
        if (_.found) {
            return _.mime;
        }
        return null;
    }

    /// Returns the guessed MIME string, or `"application/octet-stream"` if unknown.
    pub fn firstOrOctetStream(): string {
        if (_.found) {
            return _.mime;
        }
        return "application/octet-stream";
    }

    /// Returns the guessed MIME string, or `"text/plain"` if unknown.
    pub fn firstOrTextPlain(): string {
        if (_.found) {
            return _.mime;
        }
        return "text/plain";
    }

    pub fn isText(): bool {
        return _.found && mgsStartsWith(_.mime, "text/");
    }

    pub fn isImage(): bool {
        return _.found && mgsStartsWith(_.mime, "image/");
    }

    pub fn isAudio(): bool {
        return _.found && mgsStartsWith(_.mime, "audio/");
    }

    pub fn isVideo(): bool {
        return _.found && mgsStartsWith(_.mime, "video/");
    }

    pub fn isCompressible(): bool {
        return _.found && mgsIsCompressibleMime(_.mime);
    }
}
