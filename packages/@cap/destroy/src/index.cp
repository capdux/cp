import { StreamLifecycle, isFinished } from "../../on-finished/src/index.cp";

pub const STREAM_KIND_GENERIC: u8 = 0;
pub const STREAM_KIND_READ_FILE: u8 = 1;
pub const STREAM_KIND_ZLIB: u8 = 2;

pub const DESTROY_ACTION_NONE: u8 = 0;
pub const DESTROY_ACTION_DESTROYED: u8 = 1;
pub const DESTROY_ACTION_DEFERRED_CLOSE: u8 = 2;
pub const DESTROY_ACTION_ALREADY_DESTROYED: u8 = 3;

pub struct DestroyableStream {
    pub kind: u8,
    pub has_fd: bool,
    pub fd: i32,
    pub fd_open: bool,
    pub destroyed: bool,
    pub destroy_calls: usize,
    pub close_calls: usize,
    pub lifecycle: StreamLifecycle,

    pub static fn generic(): DestroyableStream {
        return DestroyableStream {
            kind: STREAM_KIND_GENERIC,
            has_fd: false,
            fd: -1,
            fd_open: false,
            destroyed: false,
            destroy_calls: 0,
            close_calls: 0,
            lifecycle: StreamLifecycle.new(),
        };
    }

    pub static fn readFile(fd: i32, fd_open: bool): DestroyableStream {
        return DestroyableStream {
            kind: STREAM_KIND_READ_FILE,
            has_fd: fd >= 0,
            fd: fd,
            fd_open: fd_open,
            destroyed: false,
            destroy_calls: 0,
            close_calls: 0,
            lifecycle: StreamLifecycle.new(),
        };
    }

    pub static fn zlib(): DestroyableStream {
        return DestroyableStream {
            kind: STREAM_KIND_ZLIB,
            has_fd: false,
            fd: -1,
            fd_open: false,
            destroyed: false,
            destroy_calls: 0,
            close_calls: 0,
            lifecycle: StreamLifecycle.new(),
        };
    }

    pub fn isDestroyed(): bool {
        return _.destroyed || isFinished(_.lifecycle);
    }

    pub fn onOpen(*_, fd: i32): u8 {
        _.has_fd = fd >= 0;
        _.fd = fd;
        _.fd_open = fd >= 0;
        if (_.destroyed && _.fd_open) {
            _.fd_open = false;
            _.close_calls = _.close_calls + 1;
            * = _.lifecycle.markClose();
            return DESTROY_ACTION_DESTROYED;
        }
        return DESTROY_ACTION_NONE;
    }
}

pub fn destroyStream(s: *DestroyableStream): u8 {
    if (s.destroyed) {
        return DESTROY_ACTION_ALREADY_DESTROYED;
    }
    s.destroyed = true;
    s.destroy_calls = s.destroy_calls + 1;

    if (s.kind == STREAM_KIND_READ_FILE) {
        if (s.has_fd && s.fd_open) {
            s.fd_open = false;
            s.close_calls = s.close_calls + 1;
            * = s.lifecycle.markClose();
            return DESTROY_ACTION_DESTROYED;
        }
        // ReadStream without opened fd yet: defer close until onOpen() fires
        return DESTROY_ACTION_DEFERRED_CLOSE;
    }

    if (s.kind == STREAM_KIND_ZLIB) {
        s.close_calls = s.close_calls + 1;
        * = s.lifecycle.markClose();
        return DESTROY_ACTION_DESTROYED;
    }

    * = s.lifecycle.markClose();
    return DESTROY_ACTION_DESTROYED;
}

pub fn destroyWithError(s: *DestroyableStream, err_code: i32): u8 {
    if (s.destroyed) {
        return DESTROY_ACTION_ALREADY_DESTROYED;
    }
    s.destroyed = true;
    s.destroy_calls = s.destroy_calls + 1;
    if (s.has_fd && s.fd_open) {
        s.fd_open = false;
        s.close_calls = s.close_calls + 1;
    }
    * = s.lifecycle.markError(err_code);
    return DESTROY_ACTION_DESTROYED;
}
