pub const MAX_LISTENERS: usize = 16;

pub const REASON_NONE: u8 = 0;
pub const REASON_END: u8 = 1;
pub const REASON_FINISH: u8 = 2;
pub const REASON_CLOSE: u8 = 3;
pub const REASON_ERROR: u8 = 4;

pub struct ListenerSlot {
    pub id: u32,
    pub fired: bool,
    pub reason: u8,
    pub err_code: i32,
}

pub struct StreamLifecycle {
    pub readable_ended: bool,
    pub writable_finished: bool,
    pub socket_closed: bool,
    pub errored: bool,
    pub err_code: i32,
    pub finish_reason: u8,
    pub listeners: [16]ListenerSlot,
    pub listener_count: usize,
    pub fired_count: usize,

    pub static fn new(): StreamLifecycle {
        let blank = ListenerSlot { id: 0, fired: false, reason: REASON_NONE, err_code: 0 };
        let arr: [16]ListenerSlot = [blank; 16];
        return StreamLifecycle {
            readable_ended: false,
            writable_finished: false,
            socket_closed: false,
            errored: false,
            err_code: 0,
            finish_reason: REASON_NONE,
            listeners: arr,
            listener_count: 0,
            fired_count: 0,
        };
    }

    pub fn isFinished(): bool {
        return _.readable_ended || _.writable_finished || _.socket_closed || _.errored;
    }

    pub fn isReadableFinished(): bool {
        return _.readable_ended || _.socket_closed || _.errored;
    }

    pub fn isWritableFinished(): bool {
        return _.writable_finished || _.socket_closed || _.errored;
    }

    pub fn onFinished(*_, listener_id: u32): bool {
        if (_.listener_count >= MAX_LISTENERS) {
            return false;
        }
        let idx = _.listener_count;
        let already = _.isFinished();
        _.listeners[idx] = ListenerSlot {
            id: listener_id,
            fired: already,
            reason: if (already) { _.finish_reason } else { REASON_NONE },
            err_code: if (already) { _.err_code } else { 0 },
        };
        _.listener_count = _.listener_count + 1;
        if (already) {
            _.fired_count = _.fired_count + 1;
        }
        return true;
    }

    pub fn dispatchAll(*_, reason: u8, err_code: i32): usize {
        if (_.finish_reason == REASON_NONE) {
            _.finish_reason = reason;
            _.err_code = err_code;
        }
        let newly_fired: usize = 0;
        let i: usize = 0;
        while (i < _.listener_count) {
            if (!_.listeners[i].fired) {
                _.listeners[i].fired = true;
                _.listeners[i].reason = _.finish_reason;
                _.listeners[i].err_code = _.err_code;
                _.fired_count = _.fired_count + 1;
                newly_fired = newly_fired + 1;
            }
            i = i + 1;
        }
        return newly_fired;
    }

    pub fn markEnd(*_): usize {
        _.readable_ended = true;
        return _.dispatchAll(REASON_END, 0);
    }

    pub fn markFinish(*_): usize {
        _.writable_finished = true;
        return _.dispatchAll(REASON_FINISH, 0);
    }

    pub fn markClose(*_): usize {
        _.socket_closed = true;
        return _.dispatchAll(REASON_CLOSE, 0);
    }

    pub fn markError(*_, code: i32): usize {
        _.errored = true;
        return _.dispatchAll(REASON_ERROR, code);
    }

    pub fn getListener(listener_id: u32): ListenerSlot? {
        let i: usize = 0;
        while (i < _.listener_count) {
            if (_.listeners[i].id == listener_id) {
                return _.listeners[i];
            }
            i = i + 1;
        }
        return null;
    }
}

pub fn isFinished(lc: StreamLifecycle): bool {
    return lc.isFinished();
}
