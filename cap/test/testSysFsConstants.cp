// Deep production regression tests for cap:sys fsConstants.cp
// Covers CopyfileFlags bitset invariants, POSIX file type predicates, permission bit queries,
// umask application, 10-char ls -l symbolic formatting (incl. setuid/setgid/sticky), and octal parsing.

import { test, expect } from "cap:test";
import {
    COPYFILE_EXCL,
    COPYFILE_FICLONE,
    COPYFILE_FICLONE_FORCE,
    COPYFILE_VALID_MASK,
    UV_FS_O_FILEMAP,
    FS_IFMT,
    FS_IFIFO,
    FS_IFCHR,
    FS_IFDIR,
    FS_IFBLK,
    FS_IFREG,
    FS_IFLNK,
    FS_IFSOCK,
    S_ISUID,
    S_ISGID,
    S_ISVTX,
    S_IRWXU,
    S_IRUSR,
    S_IWUSR,
    S_IXUSR,
    S_IRWXG,
    S_IRGRP,
    S_IWGRP,
    S_IXGRP,
    S_IRWXO,
    S_IROTH,
    S_IWOTH,
    S_IXOTH,
    FS_PERM_MASK,
    CopyfileFlags,
    FileMode,
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

test("fsConstants: CopyfileFlags bit values, builders and validation", () => {
    expect(COPYFILE_EXCL == 1);
    expect(COPYFILE_FICLONE == 2);
    expect(COPYFILE_FICLONE_FORCE == 4);
    expect(COPYFILE_VALID_MASK == 7);
    expect(UV_FS_O_FILEMAP == 536870912);

    const empty = CopyfileFlags.none();
    expect(empty.isValid());
    expect(!empty.shouldntOverwrite());
    expect(!empty.wantsClone());
    expect(!empty.isForceClone());

    const excl_clone = empty.withExclusive().withClone();
    expect(excl_clone.isValid());
    expect(excl_clone.shouldntOverwrite());
    expect(excl_clone.wantsClone());
    expect(!excl_clone.isForceClone());

    const forced = CopyfileFlags.none().withForceClone();
    expect(forced.isValid());
    expect(forced.wantsClone());
    expect(forced.isForceClone());
    expect(!forced.shouldntOverwrite());

    // Unknown high bits fail validation
    const bad = CopyfileFlags.fromBits(0x80);
    expect(!bad.isValid());
});

test("fsConstants: POSIX S_IF* file type predicates and typeChar", () => {
    expect(FS_IFMT == 0o170000);

    const reg = FileMode.fromTypeAndPerms(FS_IFREG, 0o644);
    expect(reg.isReg() && !reg.isDir() && reg.typeChar() == '-');

    const dir = FileMode.fromTypeAndPerms(FS_IFDIR, 0o755);
    expect(dir.isDir() && !dir.isReg() && dir.typeChar() == 'd');

    const lnk = FileMode.fromTypeAndPerms(FS_IFLNK, 0o777);
    expect(lnk.isSymlink() && lnk.typeChar() == 'l');

    const fifo = FileMode.fromTypeAndPerms(FS_IFIFO, 0o600);
    expect(fifo.isFifo() && fifo.typeChar() == 'p');

    const sock = FileMode.fromTypeAndPerms(FS_IFSOCK, 0o660);
    expect(sock.isSocket() && sock.typeChar() == 's');

    const chr = FileMode.fromTypeAndPerms(FS_IFCHR, 0o666);
    expect(chr.isCharDevice() && chr.typeChar() == 'c');

    const blk = FileMode.fromTypeAndPerms(FS_IFBLK, 0o660);
    expect(blk.isBlockDevice() && blk.typeChar() == 'b');

    const unknown = FileMode.init(0o644);
    expect(unknown.typeChar() == '?');
});

test("fsConstants: POSIX rwx bit constants and permission predicates", () => {
    expect(S_IRWXU == (S_IRUSR | S_IWUSR | S_IXUSR));
    expect(S_IRWXG == (S_IRGRP | S_IWGRP | S_IXGRP));
    expect(S_IRWXO == (S_IROTH | S_IWOTH | S_IXOTH));
    expect(FS_PERM_MASK == 0o7777);

    const m = FileMode.fromTypeAndPerms(FS_IFREG, 0o750);
    expect(m.canOwnerRead() && m.canOwnerWrite() && m.canOwnerExec());
    expect(m.canGroupRead() && !m.canGroupWrite() && m.canGroupExec());
    expect(!m.canOtherRead() && !m.canOtherWrite() && !m.canOtherExec());
    expect(m.rwxBits() == 0o750);
});

test("fsConstants: umask application preserves file type bits", () => {
    const dir = FileMode.fromTypeAndPerms(FS_IFDIR, 0o777);
    const masked_dir = dir.applyUmask(0o022);
    expect(masked_dir.isDir());
    expect(masked_dir.permBits() == 0o755);

    const file = FileMode.fromTypeAndPerms(FS_IFREG, 0o666);
    const masked_file = file.applyUmask(0o077);
    expect(masked_file.isReg());
    expect(masked_file.permBits() == 0o600);
});

test("fsConstants: formatSymbolic covers standard, setuid/setgid/sticky and small buffer", () => {
    let buf: [16]u8 = [0; 16];

    const s1 = FileMode.fromTypeAndPerms(FS_IFREG, 0o644).formatSymbolic(buf[0..]);
    expect(strEq(s1, "-rw-r--r--"));

    const s2 = FileMode.fromTypeAndPerms(FS_IFDIR, 0o755).formatSymbolic(buf[0..]);
    expect(strEq(s2, "drwxr-xr-x"));

    // Lowercase s/s/t when exec bit is also set (e.g. /usr/bin/passwd, /tmp)
    const s3 = FileMode.fromTypeAndPerms(FS_IFREG, S_ISUID | S_ISGID | S_ISVTX | 0o755).formatSymbolic(buf[0..]);
    expect(strEq(s3, "-rwsr-sr-t"));

    // Uppercase S/S/T when exec bit is NOT set
    const s4 = FileMode.fromTypeAndPerms(FS_IFDIR, S_ISUID | S_ISGID | S_ISVTX | 0o644).formatSymbolic(buf[0..]);
    expect(strEq(s4, "drwSr-Sr-T"));

    // Too-small buffer returns empty slice safely without OOB
    let tiny: [9]u8 = [0; 9];
    expect(FileMode.fromTypeAndPerms(FS_IFREG, 0o644).formatSymbolic(tiny[0..]).len == 0);
});

test("fsConstants: parseOctal handles standard prefixes and rejects invalid inputs", () => {
    expect(FileMode.parseOctal("755").? == 0o755);
    expect(FileMode.parseOctal("0755").? == 0o755);
    expect(FileMode.parseOctal("0o644").? == 0o644);
    expect(FileMode.parseOctal("0O600").? == 0o600);
    expect(FileMode.parseOctal("4755").? == 0o4755);
    expect(FileMode.parseOctal("7777").? == 0o7777);

    // Invalid inputs cleanly rejected
    expect(FileMode.parseOctal("") == null);
    expect(FileMode.parseOctal("0o") == null);
    expect(FileMode.parseOctal("0888") == null);
    expect(FileMode.parseOctal("rwxr-xr-x") == null);
    expect(FileMode.parseOctal("10000") == null);
});
