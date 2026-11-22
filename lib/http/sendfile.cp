// Source: Absorbed from /data/dev/bun/src/http/SendFile.cp | Synced: 2026-11-22T11:12:13Z
// lib/http/sendfile.cp
// CP HTTP SendFile Kernel Zero-Copy File Upload (cap:http)

const std = @import("std");

pub enum SendFileStatus {
    Done,
    Again,
    Error,
}

pub struct SendFile {
    pub fd: i32 = -1;
    pub remain: usize = 0;
    pub offset: usize = 0;
    pub content_size: usize = 0;

    pub static fn init(fd: i32, size: usize): SendFile {
        return SendFile{
            fd: fd,
            remain: size,
            offset: 0,
            content_size: size,
        };
    }

    pub fn isDone(): bool {
        return _.remain == 0;
    }

    pub fn writeToSocket(*_, socket_fd: i32): SendFileStatus {
        if (_.remain == 0) return .Done;

        let signed_offset: i64 = _.offset as i64;
        const val = std.os.linux.sendfile(socket_fd, _.fd, &signed_offset, _.remain);
        _.offset = signed_offset as usize;

        if (val > 0) {
            const sent: usize = val as usize;
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
}
