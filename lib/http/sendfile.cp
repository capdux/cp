// Source: Bun path-not-recorded
/**
 * CP HTTP SendFile Kernel Zero-Copy File Upload (cap:http)
 * Ported from Bun.js http/SendFile.cp
 */


pub const SendFileStatus = enum {
    Done,
    Again,
    Error,
};

pub const SendFile = struct {
    fd: i32 = -1,
    remain: usize = 0,
    offset: usize = 0,
    content_size: usize = 0,

    pub fn init(fd: i32, size: usize) SendFile {
        let sf = SendFile{};
        sf.fd = fd;
        sf.remain = size;
        sf.offset = 0;
        sf.content_size = size;
        return sf;
    }

    pub fn isDone(*_): bool {
        return _.remain == 0;
    }

    pub fn writeToSocket(*_, socket_fd: i32) SendFileStatus {
        if (_.remain == 0) return .Done;

        let signed_offset: i64 = @as(i64, @intCast(_.offset));
        const val = std.os.linux.sendfile(socket_fd, _.fd, &signed_offset, _.remain);
        _.offset = @as(usize, @intCast(signed_offset));

        if (val > 0) {
            const sent: usize = @as(usize, @intCast(val));
            if (sent >= _.remain) {
                _.remain = 0;
                return .Done;
            } else {
                _.remain -= sent;
                return .Again;
            }
        } else if (val == 0) {
            return .Done;
        } else {
            return .Error;
        }
    }
};
