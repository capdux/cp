// Source: Absorbed from /data/dev/bun/src/watcher/INotifyWatcher.cp | Synced: 2026-11-23T01:34:34Z
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

        const raw_fd = std.os.linux.inotify_init1(std.os.linux.IN.NONBLOCK | std.os.linux.IN.CLOEXEC);
        const signed_fd: isize = @bitCast(raw_fd);
        if (signed_fd >= 0) {
            backend.fd = @intCast(raw_fd);
            backend.is_available = true;
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

        const raw_wd = unsafe {
            std.os.linux.inotify_add_watch(@intCast(_.fd), @as([*:0]const u8, @ptrCast(&c_path)), mask)
        };
        const signed_wd: isize = @bitCast(raw_wd);
        if (signed_wd >= 0) {
            return @intCast(raw_wd);
        }
        return -1;
    }

    pub fn removeWatch(*_, wd: i32): bool {
        if (!_.is_available || _.fd < 0 || wd < 0) {
            return false;
        }

        const rc = std.os.linux.inotify_rm_watch(@intCast(_.fd), @intCast(wd));
        const signed_rc: isize = @bitCast(rc);
        return signed_rc == 0;
    }

    pub fn readEvents(*_, out_raw: *[]InotifyRawEvent): usize {
        if (!_.is_available || _.fd < 0 || out_raw.len == 0) {
            return 0;
        }

        const rc = std.os.linux.read(@intCast(_.fd), &_.buffer, _.buffer.len);
        const bytes_read: isize = @bitCast(rc);

        if (bytes_read <= 0) {
            return 0;
        }

        let total_bytes: usize = usize(bytes_read);
        let offset: usize = 0;
        let count: usize = 0;

        while (offset + 16 <= total_bytes && count < out_raw.len) {
            const wd: i32 = std.mem.readInt(i32, _.buffer[offset..offset + 4][0..4], .little);
            const mask: u32 = std.mem.readInt(u32, _.buffer[offset + 4..offset + 8][0..4], .little);
            const cookie: u32 = std.mem.readInt(u32, _.buffer[offset + 8..offset + 12][0..4], .little);
            const ev_len: u32 = std.mem.readInt(u32, _.buffer[offset + 12..offset + 16][0..4], .little);

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
            * = std.os.linux.close(@intCast(_.fd));
            _.fd = -1;
            _.is_available = false;
        }
    }

    pub fn isAvailable(): bool {
        return _.is_available;
    }
}
