// Source: Absorbed from /data/dev/bun/src/watcher/Watcher.cp | Synced: 2026-11-20T11:19:58Z
// lib/watcher/filter.cp
// Filesystem Watch Path Ignore Filter & Timestamp-Based Event Debouncer
// Absorbed from Bun src/watcher/

const std = @import("std");

pub struct WatchFilter {
    /// Returns `true` if `path` matches common VCS, package cache, build output, or editor swap patterns.
    pub static fn shouldIgnore(path: []const u8): bool {
        if (path.len == 0) {
            return true;
        }
        // Editor backup / swap files
        if (path[path.len - 1] == '~' ||
            std.mem.endsWith(u8, path, ".swp") ||
            std.mem.endsWith(u8, path, ".swo") ||
            std.mem.endsWith(u8, path, ".tmp") ||
            std.mem.endsWith(u8, path, ".DS_Store"))
        {
            return true;
        }

        // Segment-level directory filters (.git, node_modules, .cap)
        let iter = std.mem.splitScalar(u8, path, '/');
        while (seg := iter.next()) {
            if (seg.len == 0) {
                continue;
            }
            if (std.mem.eql(u8, seg, ".git") ||
                std.mem.eql(u8, seg, "node_modules") ||
                std.mem.eql(u8, seg, ".cap") ||
                std.mem.eql(u8, seg, "zig-cache"))
            {
                return true;
            }
        }
        return false;
    }

    /// Returns `true` if any path component (except `.` or `..`) starts with `.`.
    pub static fn isHidden(path: []const u8): bool {
        let iter = std.mem.splitScalar(u8, path, '/');
        while (seg := iter.next()) {
            if (seg.len == 0 || std.mem.eql(u8, seg, ".") || std.mem.eql(u8, seg, "..")) {
                continue;
            }
            if (seg[0] == '.') {
                return true;
            }
        }
        return false;
    }

    /// Simple glob/segment pattern matcher supporting `*` wildcard, prefix/suffix, and directory segment matches.
    pub static fn matchesPattern(path: []const u8, pattern: []const u8): bool {
        if (pattern.len == 0) return false;
        if (std.mem.indexOfScalar(u8, pattern, '*') == null) {
            if (std.mem.eql(u8, path, pattern)) return true;
            let iter = std.mem.splitScalar(u8, path, '/');
            while (seg := iter.next()) {
                if (std.mem.eql(u8, seg, pattern)) return true;
            }
            return std.mem.endsWith(u8, path, pattern);
        }
        // Single-star wildcard match against basename and full path
        let basename = path;
        if (last_slash := std.mem.lastIndexOfScalar(u8, path, '/')) {
            basename = path[last_slash + 1 ..];
        }
        if (matchStar(basename, pattern) || matchStar(path, pattern)) {
            return true;
        }
        return false;
    }

    /// Evaluates default ignores, hidden files, and custom `patterns`.
    pub static fn shouldIgnoreWith(
        path: []const u8,
        ignore_defaults: bool,
        ignore_hidden: bool,
        patterns: []const []const u8,
    ): bool {
        if (path.len == 0) return true;
        if (ignore_defaults && WatchFilter.shouldIgnore(path)) return true;
        if (ignore_hidden && WatchFilter.isHidden(path)) return true;
        let i: usize = 0;
        while (i < patterns.len) {
            if (WatchFilter.matchesPattern(path, patterns[i])) return true;
            i += 1;
        }
        return false;
    }
}

fn matchStar(text: []const u8, pattern: []const u8): bool {
    let ti: usize = 0;
    let pi: usize = 0;
    let star_pi: (usize)? = null;
    let match_ti: usize = 0;

    while (ti < text.len) {
        if (pi < pattern.len && pattern[pi] == text[ti]) {
            pi += 1;
            ti += 1;
        } else if (pi < pattern.len && pattern[pi] == '*') {
            star_pi = pi;
            match_ti = ti;
            pi += 1;
        } else if (sp := star_pi) {
            pi = sp + 1;
            match_ti += 1;
            ti = match_ti;
        } else {
            return false;
        }
    }
    while (pi < pattern.len && pattern[pi] == '*') {
        pi += 1;
    }
    return pi == pattern.len;
}

pub struct TrackerEntry {
    path: []const u8 = "";
    last_ns: u64 = 0;
    active: bool = false;
}

/// Fixed-capacity timestamp debouncer that suppresses duplicate file events within `window_ns`.
pub struct DebounceTracker<const CAP: usize> {
    entries: [CAP]TrackerEntry = undefined;
    window_ns: u64 = 50000000; // default 50ms
    count: usize = 0;
    next_evict: usize = 0;

    pub static fn init(window_ns: u64): DebounceTracker<CAP> {
        let tracker = DebounceTracker<CAP> {
            entries: undefined,
            window_ns: window_ns,
            count: 0,
            next_evict: 0,
        };
        let i: usize = 0;
        while (i < CAP) {
            tracker.entries[i] = TrackerEntry {
                path: "",
                last_ns: 0,
                active: false,
            };
            i += 1;
        }
        return tracker;
    }

    /// Checks whether an event on `path` at `now_ns` should be emitted.
    /// Returns `true` and updates the timestamp if outside the debounce window; returns `false` if suppressed.
    pub fn shouldEmit(*_, path: []const u8, now_ns: u64): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].path, path)) {
                const prev = _.entries[i].last_ns;
                if (now_ns >= prev && (now_ns - prev) < _.window_ns) {
                    return false;
                }
                _.entries[i].last_ns = now_ns;
                return true;
            }
            i += 1;
        }

        if (_.count < CAP) {
            _.entries[_.count] = TrackerEntry {
                path: path,
                last_ns: now_ns,
                active: true,
            };
            _.count += 1;
            return true;
        }

        // Ring eviction when full
        const slot = _.next_evict;
        _.next_evict = (_.next_evict + 1) % CAP;
        _.entries[slot] = TrackerEntry {
            path: path,
            last_ns: now_ns,
            active: true,
        };
        return true;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < CAP) {
            _.entries[i].active = false;
            i += 1;
        }
        _.count = 0;
        _.next_evict = 0;
    }
}
