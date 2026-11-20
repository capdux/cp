// Source: /data/dev/bun/src/runtime/node/dir_iterator.cp & /data/dev/bun/src/sys/dir.cp
// ============================================================================
// lib/sys/dir.cp — Zero-Heap Linux getdents64 Directory Iterator & Dir Handle
// ============================================================================

const std = @import("std");
import { FD, O_RDONLY, O_DIRECTORY, O_CLOEXEC, AT_FDCWD } from "./fd";
import { SysError, Maybe, linuxSyscallErrno, EINTR, ENAMETOOLONG } from "./error";

pub enum DirEntryKind: u8 {
    file,
    directory,
    sym_link,
    block_device,
    character_device,
    named_pipe,
    unix_domain_socket,
    whiteout,
    unknown,
}

pub struct DirEntry {
    name: []const u8 = "",
    kind: DirEntryKind = .unknown,
    ino: u64 = 0,
}

pub type DirEntryOpt = DirEntry?;

fn dtToKind(dt: u8): DirEntryKind {
    if (dt == std.os.linux.DT.REG) return .file;
    if (dt == std.os.linux.DT.DIR) return .directory;
    if (dt == std.os.linux.DT.LNK) return .sym_link;
    if (dt == std.os.linux.DT.FIFO) return .named_pipe;
    if (dt == std.os.linux.DT.SOCK) return .unix_domain_socket;
    if (dt == std.os.linux.DT.BLK) return .block_device;
    if (dt == std.os.linux.DT.CHR) return .character_device;
    if (dt == std.os.linux.DT.WHT) return .whiteout;
    return .unknown;
}

/// Zero-heap-allocation buffered directory iterator backed by Linux `getdents64`.
/// Returned `DirEntry.name` slice points into the internal 8KB buffer and remains
/// valid until the next `next()` call or `close()`.
pub struct DirIterator {
    dir_fd: FD = FD.invalid,
    owns_fd: bool = false,
    @align(8) buf: [8192]u8 = undefined,
    index: usize = 0,
    end_index: usize = 0,
    done: bool = false,

    static fn fromFd(dir_fd: FD): DirIterator {
        return DirIterator{
            dir_fd: dir_fd,
            owns_fd: false,
            buf: undefined,
            index: 0,
            end_index: 0,
            done: false,
        };
    }

    static fn open(path: []const u8): !DirIterator {
        return DirIterator.openAt(FD.cwd(), path);
    }

    static fn openAt(parent_fd: FD, path: []const u8): !DirIterator {
        let path_buf: [4096]u8 = undefined;
        if (path.len >= path_buf.len) return error.NameTooLong;
        @memcpy(path_buf[0 .. path.len], path);
        path_buf[path.len] = 0;
        const path_z: [*:0]const u8 = path_buf[0 .. path.len :0].ptr;
        const parent_raw = if (parent_fd.isValid()) parent_fd.raw else AT_FDCWD;

        while (true) {
            const flags = std.os.linux.O{
                .ACCMODE = .RDONLY,
                .DIRECTORY = true,
                .CLOEXEC = true,
            };
            const rc = std.os.linux.openat(parent_raw, path_z, flags, 0);
            const err_code = linuxSyscallErrno(rc);
            if (err_code == 0) {
                return DirIterator{
                    dir_fd: FD.fromNative(@intCast(rc)),
                    owns_fd: true,
                    buf: undefined,
                    index: 0,
                    end_index: 0,
                    done: false,
                };
            }
            if (err_code == EINTR) continue;
            return SysError.fromCode(err_code, .openat).withPath(path).toError();
        }
    }

    fn nextMaybe(*_): Maybe<DirEntryOpt> {
        if (_.done) return Maybe<DirEntryOpt>.initResult(null);
        if (!_.dir_fd.isValid()) {
            return Maybe<DirEntryOpt>.initErr(SysError.fromCode(9, .getdents64).withFd(_.dir_fd.raw));
        }

        while (true) {
            if (_.index >= _.end_index) {
                while (true) {
                    const rc = std.os.linux.getdents64(_.dir_fd.raw, &_.buf, _.buf.len);
                    const err_code = linuxSyscallErrno(rc);
                    if (err_code == EINTR) continue;
                    if (err_code != 0) {
                        return Maybe<DirEntryOpt>.initErr(SysError.fromCode(err_code, .getdents64).withFd(_.dir_fd.raw));
                    }
                    if (rc == 0) {
                        _.done = true;
                        return Maybe<DirEntryOpt>.initResult(null);
                    }
                    _.index = 0;
                    _.end_index = rc;
                    break;
                }
            }

            let entry_ino: u64 = 0;
            let entry_reclen: u16 = 0;
            let entry_type: u8 = 0;
            let entry_name: []const u8 = "";

            zig {
                const linux_entry: *align(1) const std.os.linux.dirent64 = @ptrCast(&self.buf[self.index]);
                entry_ino = linux_entry.ino;
                entry_reclen = linux_entry.reclen;
                entry_type = linux_entry.type;
                const name_ptr: [*:0]const u8 = @ptrCast(&linux_entry.name);
                entry_name = std.mem.sliceTo(name_ptr, 0);
            }

            if (entry_reclen == 0) {
                _.done = true;
                return Maybe<DirEntryOpt>.initResult(null);
            }
            _.index += (entry_reclen as usize);

            // Skip "." and ".."
            if (std.mem.eql(u8, entry_name, ".") || std.mem.eql(u8, entry_name, "..")) {
                continue;
            }

            return Maybe<DirEntryOpt>.initResult(DirEntry{
                name: entry_name,
                kind: dtToKind(entry_type),
                ino: entry_ino,
            });
        }
    }

    fn next(*_): !DirEntry? {
        return _.nextMaybe().unwrap();
    }

    fn reset(*_) {
        if (_.dir_fd.isValid()) {
            * = _.dir_fd.lseek(0, 0) catch {};
        }
        _.index = 0;
        _.end_index = 0;
        _.done = false;
    }

    fn close(*_) {
        if (_.owns_fd && _.dir_fd.isValid()) {
            _.dir_fd.close();
            _.dir_fd = FD.invalid;
            _.owns_fd = false;
        }
        _.done = true;
    }
}

pub struct Dir {
    fd: FD = FD.invalid,

    static fn open(path: []const u8): !Dir {
        return Dir.openAt(FD.cwd(), path);
    }

    static fn openAt(parent_fd: FD, path: []const u8): !Dir {
        const iter = try DirIterator.openAt(parent_fd, path);
        return Dir{ fd: iter.dir_fd };
    }

    static fn fromFd(fd: FD): Dir {
        return Dir{ fd: fd };
    }

    fn iterate(): DirIterator {
        return DirIterator.fromFd(_.fd);
    }

    fn close(*_) {
        _.fd.close();
        _.fd = FD.invalid;
    }
}
