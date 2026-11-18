// Source: bun/src/watcher/INotifyWatcher.cp
import std from "std";

pub const IN_ACCESS: u32 = 0x00000001;
pub const IN_MODIFY: u32 = 0x00000002;
pub const IN_ATTRIB: u32 = 0x00000004;
pub const IN_CLOSE_WRITE: u32 = 0x00000008;
pub const IN_CLOSE_NOWRITE: u32 = 0x00000010;
pub const IN_OPEN: u32 = 0x00000020;
pub const IN_MOVED_FROM: u32 = 0x00000040;
pub const IN_MOVED_TO: u32 = 0x00000080;
pub const IN_CREATE: u32 = 0x00000100;
pub const IN_DELETE: u32 = 0x00000200;
pub const IN_DELETE_SELF: u32 = 0x00000400;
pub const IN_MOVE_SELF: u32 = 0x00000800;
pub const IN_ISDIR: u32 = 0x40000000;
pub const IN_NONBLOCK: u32 = 0x00000800;
pub const IN_CLOEXEC: u32 = 0x00080000;

pub const DEFAULT_WATCH_MASK: u32 =
    IN_MODIFY |
    IN_ATTRIB |
    IN_MOVED_FROM |
    IN_MOVED_TO |
    IN_CREATE |
    IN_DELETE |
    IN_DELETE_SELF |
    IN_MOVE_SELF;

pub struct InotifyRawEvent {
    pub wd: i32 = -1,
    pub mask: u32 = 0,
    pub cookie: u32 = 0,
    pub len: u32 = 0,
    pub name_buf: [256]u8 = undefined,
    pub name_len: usize = 0,

    pub fn name(*_): []const u8 {
        return _.name_buf[0.._.name_len];
    }
}

pub struct InotifyBackend {
    pub fd: i32 = -1,
    pub is_available: bool = false,
    pub buffer: [4096]u8 = undefined,

    pub static fn init(): InotifyBackend {
        let backend = InotifyBackend{
            fd: -1,
            is_available: false,
            buffer: undefined,
        };

        __zig__ {
            const raw_fd = std.os.linux.inotify_init1(std.os.linux.IN.NONBLOCK | std.os.linux.IN.CLOEXEC);
            const signed_fd: isize = @bitCast(raw_fd);
            if (signed_fd >= 0) {
                backend.fd = @intCast(raw_fd);
                backend.is_available = true;
            }
        }

        return backend;
    }

    pub fn addWatch(*_, path: []const u8, mask: u32): i32 {
        if (!_.is_available || _.fd < 0 || path.len >= 512) {
            return -1;
        }

        let c_path: [512]u8 = undefined;
        @memcpy(c_path[0..path.len], path);
        c_path[path.len] = 0;

        let fd = _.fd;
        let res: i32 = -1;
        __zig__ {
            const raw_wd = std.os.linux.inotify_add_watch(@intCast(fd), @as([*:0]const u8, @ptrCast(&c_path)), mask);
            const signed_wd: isize = @bitCast(raw_wd);
            if (signed_wd >= 0) {
                res = @intCast(raw_wd);
            }
        }
        return res;
    }

    pub fn removeWatch(*_, wd: i32): bool {
        if (!_.is_available || _.fd < 0 || wd < 0) {
            return false;
        }

        let fd = _.fd;
        let success: bool = false;
        __zig__ {
            const rc = std.os.linux.inotify_rm_watch(@intCast(fd), @intCast(wd));
            const signed_rc: isize = @bitCast(rc);
            success = (signed_rc == 0);
        }
        return success;
    }

    pub fn readEvents(*_, out_raw: *[]InotifyRawEvent): usize {
        if (!_.is_available || _.fd < 0 || out_raw.len == 0) {
            return 0;
        }

        let fd = _.fd;
        let buf_ptr = &_.buffer;
        let buf_len = _.buffer.len;
        let bytes_read: isize = -1;
        __zig__ {
            const rc = std.os.linux.read(@intCast(fd), buf_ptr, buf_len);
            bytes_read = @bitCast(rc);
        }

        if (bytes_read <= 0) {
            return 0;
        }

        let total_bytes: usize = usize(bytes_read);
        let offset: usize = 0;
        let count: usize = 0;

        while (offset + 16 <= total_bytes && count < out_raw.len) {
            let wd: i32 = 0;
            let mask: u32 = 0;
            let cookie: u32 = 0;
            let ev_len: u32 = 0;

            __zig__ {
                wd = @as(*const align(1) i32, @ptrCast(&buf_ptr[offset])).*;
                mask = @as(*const align(1) u32, @ptrCast(&buf_ptr[offset + 4])).*;
                cookie = @as(*const align(1) u32, @ptrCast(&buf_ptr[offset + 8])).*;
                ev_len = @as(*const align(1) u32, @ptrCast(&buf_ptr[offset + 12])).*;
            }

            out_raw[count].wd = wd;
            out_raw[count].mask = mask;
            out_raw[count].cookie = cookie;
            out_raw[count].len = ev_len;
            out_raw[count].name_len = 0;

            offset += 16;
            if (ev_len > 0) {
                let actual_len: usize = 0;
                while (actual_len < usize(ev_len) && offset + actual_len < total_bytes) {
                    if (_.buffer[offset + actual_len] == 0) {
                        break;
                    }
                    actual_len += 1;
                }
                if (actual_len > 256) {
                    actual_len = 256;
                }
                if (actual_len > 0) {
                    @memcpy(out_raw[count].name_buf[0..actual_len], _.buffer[offset..offset + actual_len]);
                    out_raw[count].name_len = actual_len;
                }
                offset += usize(ev_len);
            }
            count += 1;
        }
        return count;
    }

    pub fn close(*_) {
        if (_.is_available && _.fd >= 0) {
            let fd = _.fd;
            __zig__ {
                _ = std.os.linux.close(@intCast(fd));
            }
            _.fd = -1;
            _.is_available = false;
        }
    }

    pub fn isAvailable(): bool {
        return _.is_available;
    }
}
