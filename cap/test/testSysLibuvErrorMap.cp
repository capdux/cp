// Deep production regression tests for cap:sys libuvErrorMap.cp
// Covers all 84 uv.h entries, POSIX & EAI_* DNS diagnostics, case-insensitive lookup,
// retryable/category classification, Node.js error formatting & small-buffer truncation safety.

import { test, expect } from "cap:test";
import {
    LIBUV_ERROR_COUNT,
    LIBUV_UNKNOWN_MESSAGE,
    LibuvErrorCategory,
    LibuvErrno,
    LibuvErrorEntry,
    LibuvErrorMap,
} from "cap:sys";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("libuvErrorMap: full 84-entry table invariant and index alignment", () => {
    expect(LibuvErrorMap.entryCount() == 84);
    expect(LIBUV_ERROR_COUNT == 84);

    let i: usize = 0;
    while (i < LIBUV_ERROR_COUNT) {
        const entry = LibuvErrorMap.entryAt(i);
        expect(entry != null);
        if (e := entry) {
            expect(@intFromEnum(e.code) == i);
            expect(e.name.len > 0);
            expect(e.message.len > 0);
            // Round-trip by enum and by name must match identically
            const by_code = LibuvErrorMap.entryFor(e.code);
            expect(strEq(by_code.name, e.name));
            expect(strEq(by_code.message, e.message));

            const by_name = LibuvErrorMap.lookupByName(e.name);
            expect(by_name != null);
            if (bn := by_name) {
                expect(bn.code == e.code);
                expect(strEq(bn.message, e.message));
            }
        }
        i += 1;
    }

    // Out-of-bounds index safely returns null
    expect(LibuvErrorMap.entryAt(LIBUV_ERROR_COUNT) == null);
    expect(LibuvErrorMap.entryAt(999) == null);
});

test("libuvErrorMap: canonical POSIX & uv.h error descriptions match upstream Bun", () => {
    expect(strEq(LibuvErrorMap.messageFor(.ENOENT), "no such file or directory"));
    expect(strEq(LibuvErrorMap.messageFor(.EACCES), "permission denied"));
    expect(strEq(LibuvErrorMap.messageFor(.EADDRINUSE), "address already in use"));
    expect(strEq(LibuvErrorMap.messageFor(.EAGAIN), "resource temporarily unavailable"));
    expect(strEq(LibuvErrorMap.messageFor(.ECONNREFUSED), "connection refused"));
    expect(strEq(LibuvErrorMap.messageFor(.ECONNRESET), "connection reset by peer"));
    expect(strEq(LibuvErrorMap.messageFor(.EEXIST), "file already exists"));
    expect(strEq(LibuvErrorMap.messageFor(.EINTR), "interrupted system call"));
    expect(strEq(LibuvErrorMap.messageFor(.EINVAL), "invalid argument"));
    expect(strEq(LibuvErrorMap.messageFor(.EMFILE), "too many open files"));
    expect(strEq(LibuvErrorMap.messageFor(.ENOSPC), "no space left on device"));
    expect(strEq(LibuvErrorMap.messageFor(.EPERM), "operation not permitted"));
    expect(strEq(LibuvErrorMap.messageFor(.EPIPE), "broken pipe"));
    expect(strEq(LibuvErrorMap.messageFor(.ETIMEDOUT), "connection timed out"));
    expect(strEq(LibuvErrorMap.messageFor(.EOF), "end of file"));
    expect(strEq(LibuvErrorMap.messageFor(.UNKNOWN), "unknown error"));
    expect(strEq(LibuvErrorMap.messageFor(.EUNATCH), "protocol driver not attached"));

    // Method receiver helpers on LibuvErrno
    expect(strEq(LibuvErrno.ENOENT.codeName(), "ENOENT"));
    expect(strEq(LibuvErrno.ENOENT.message(), "no such file or directory"));
});

test("libuvErrorMap: DNS EAI_* entries and category predicates", () => {
    expect(LibuvErrno.EAI_AGAIN.isDnsError());
    expect(LibuvErrno.EAI_NONAME.isDnsError());
    expect(LibuvErrno.EAI_NODATA.isDnsError());
    expect(LibuvErrno.EAI_SERVICE.isDnsError());
    expect(LibuvErrno.EAI_ADDRFAMILY.isDnsError());
    expect(LibuvErrno.EAI_SOCKTYPE.isDnsError());
    expect(!LibuvErrno.ENOENT.isDnsError());
    expect(!LibuvErrno.ECONNRESET.isDnsError());

    expect(strEq(LibuvErrno.EAI_AGAIN.message(), "temporary failure"));
    expect(strEq(LibuvErrno.EAI_NONAME.message(), "unknown node or service"));
    expect(strEq(LibuvErrno.EAI_NODATA.message(), "no address"));
    expect(strEq(LibuvErrno.EAI_MEMORY.message(), "out of memory"));

    expect(LibuvErrno.EAI_NONAME.category() == .Dns);
    expect(strEq(LibuvErrno.EAI_NONAME.category().asStr(), "dns"));
    expect(LibuvErrno.ENOENT.category() == .Filesystem);
    expect(strEq(LibuvErrno.ENOENT.category().asStr(), "filesystem"));
    expect(LibuvErrno.EACCES.category() == .Permission);
    expect(strEq(LibuvErrno.EACCES.category().asStr(), "permission"));
    expect(LibuvErrno.ECONNREFUSED.category() == .Network);
    expect(strEq(LibuvErrno.ECONNREFUSED.category().asStr(), "network"));
    expect(LibuvErrno.UNKNOWN.category() == .Unknown);
    expect(strEq(LibuvErrno.UNKNOWN.category().asStr(), "unknown"));
});

test("libuvErrorMap: retryability classification for transient vs permanent errors", () => {
    // Transient errors should be retryable
    expect(LibuvErrorMap.isRetryable(.EAGAIN));
    expect(LibuvErrorMap.isRetryable(.EINTR));
    expect(LibuvErrorMap.isRetryable(.EBUSY));
    expect(LibuvErrorMap.isRetryable(.EAI_AGAIN));
    expect(LibuvErrorMap.isRetryable(.ECONNRESET));
    expect(LibuvErrorMap.isRetryable(.ECONNREFUSED));
    expect(LibuvErrorMap.isRetryable(.ETIMEDOUT));
    expect(LibuvErrorMap.isRetryable(.EMFILE));
    expect(LibuvErrno.ETXTBSY.isRetryable());

    // Permanent errors must not be marked retryable
    expect(!LibuvErrorMap.isRetryable(.ENOENT));
    expect(!LibuvErrorMap.isRetryable(.EACCES));
    expect(!LibuvErrorMap.isRetryable(.EPERM));
    expect(!LibuvErrorMap.isRetryable(.EINVAL));
    expect(!LibuvErrorMap.isRetryable(.EISDIR));
    expect(!LibuvErrorMap.isRetryable(.ENOTDIR));
    expect(!LibuvErrorMap.isRetryable(.EAI_NONAME));
    expect(!LibuvErrorMap.isRetryable(.EOF));
});

test("libuvErrorMap: name lookup (exact, case-insensitive, unknown fallback)", () => {
    // Exact lookup
    const code = LibuvErrorMap.fromCodeName("ECONNRESET");
    expect(code != null);
    if (c := code) {
        expect(c == .ECONNRESET);
    }

    // Case-sensitive miss vs case-insensitive hit
    expect(LibuvErrorMap.lookupByName("enoent") == null);
    const ci = LibuvErrorMap.lookupByNameIgnoreCase("eNoEnT");
    expect(ci != null);
    if (e := ci) {
        expect(e.code == .ENOENT);
        expect(strEq(e.message, "no such file or directory"));
    }

    // Empty and unknown names fall back safely
    expect(LibuvErrorMap.lookupByName("") == null);
    expect(LibuvErrorMap.lookupByNameIgnoreCase("") == null);
    expect(LibuvErrorMap.fromCodeName("ENOTREAL") == null);
    expect(strEq(LibuvErrorMap.messageByName("ENOTREAL"), LIBUV_UNKNOWN_MESSAGE));
    expect(strEq(LibuvErrorMap.messageByName("EPIPE"), "broken pipe"));
});

test("libuvErrorMap: Node.js formatError combinations and safe buffer truncation", () => {
    let buf: [160]u8 = [0; 160];

    // 1. Code + syscall + path
    const s1 = LibuvErrorMap.formatError(.ENOENT, "open", "/tmp/data.bin", buf[0..]);
    expect(strEq(s1, "ENOENT: no such file or directory, open '/tmp/data.bin'"));

    // 2. Code + syscall only
    const s2 = LibuvErrorMap.formatError(.EADDRINUSE, "listen", "", buf[0..]);
    expect(strEq(s2, "EADDRINUSE: address already in use, listen"));

    // 3. Code + path only
    const s3 = LibuvErrorMap.formatError(.EACCES, "", "/root/secret", buf[0..]);
    expect(strEq(s3, "EACCES: permission denied, '/root/secret'"));

    // 4. Code only
    const s4 = LibuvErrorMap.formatError(.EOF, "", "", buf[0..]);
    expect(strEq(s4, "EOF: end of file"));

    // 5. Tiny buffer truncation safety (no OOB)
    let tiny: [10]u8 = [0; 10];
    const s5 = LibuvErrorMap.formatError(.ENOENT, "open", "/tmp/x", tiny[0..]);
    expect(s5.len == 10);
    expect(strEq(s5, "ENOENT: no"));

    // 6. Zero-length buffer safety
    let empty: [0]u8 = [];
    const s6 = LibuvErrorMap.formatError(.EIO, "read", "", empty[0..]);
    expect(s6.len == 0);
});
