// Source: Absorbed from /data/dev/bun/src/event_loop/AnyEventLoop.cp | Synced: 2026-11-23T21:43:11Z
// lib/event/file_poll.cp
// Cross-platform I/O file descriptor poller based on std.os.linux.poll

const std = @import("std");

pub struct PollEvents {
    pub const Readable: u32 = 1;
    pub const Writable: u32 = 2;
    pub const Error: u32 = 4;
    pub const Hup: u32 = 8;
}

pub type PollCallback = fn(fd: i32, events: u32, ctx: *!?);

pub struct FilePollRegistration {
    pub fd: i32 = -1;
    pub events: u32 = 0;
    pub callback: PollCallback = undefined;
    pub ctx: *!? = null;
}

pub class PollEngine {
    regs: []FilePollRegistration = [];
    poll_fds: []std.os.linux.pollfd = [];
    capacity: usize = 0;
    len: usize = 0;

    pub static fn init(): *PollEngine {
        let pe = std.heap.page_allocator.create(PollEngine) catch unreachable;
        pe.capacity = 32;
        pe.len = 0;
        pe.regs = std.heap.page_allocator.alloc(FilePollRegistration, 32) catch unreachable;
        pe.poll_fds = std.heap.page_allocator.alloc(std.os.linux.pollfd, 32) catch unreachable;
        return pe;
    }

    pub fn count(): usize {
        return _.len;
    }

    pub fn register(*_, fd: i32, events: u32, cb: PollCallback, ctx: *!?): bool {
        // If already registered, update
        for (i of 0.._.len) {
            if (_.regs[i].fd == fd) {
                _.regs[i].events = events;
                _.regs[i].callback = cb;
                _.regs[i].ctx = ctx;
                let z_events: i16 = 0;
                if ((events & PollEvents.Readable) != 0) z_events |= std.os.linux.POLL.IN;
                if ((events & PollEvents.Writable) != 0) z_events |= std.os.linux.POLL.OUT;
                _.poll_fds[i].events = z_events;
                return true;
            }
        }

        if (_.len >= _.capacity) {
            const new_cap = _.capacity * 2;
            let new_regs = std.heap.page_allocator.alloc(FilePollRegistration, new_cap) catch unreachable;
            let new_fds = std.heap.page_allocator.alloc(std.os.linux.pollfd, new_cap) catch unreachable;
            for (i of 0.._.len) {
                new_regs[i] = _.regs[i];
                new_fds[i] = _.poll_fds[i];
            }
            std.heap.page_allocator.free(_.regs);
            std.heap.page_allocator.free(_.poll_fds);
            _.regs = new_regs;
            _.poll_fds = new_fds;
            _.capacity = new_cap;
        }

        const idx = _.len;
        _.len += 1;
        _.regs[idx].fd = fd;
        _.regs[idx].events = events;
        _.regs[idx].callback = cb;
        _.regs[idx].ctx = ctx;

        let z_events: i16 = 0;
        if ((events & PollEvents.Readable) != 0) z_events |= std.os.linux.POLL.IN;
        if ((events & PollEvents.Writable) != 0) z_events |= std.os.linux.POLL.OUT;

        _.poll_fds[idx].fd = fd;
        _.poll_fds[idx].events = z_events;
        _.poll_fds[idx].revents = 0;
        return true;
    }

    pub fn unregister(*_, fd: i32): bool {
        if (_.len == 0) return false;
        for (i of 0.._.len) {
            if (_.regs[i].fd == fd) {
                _.len -= 1;
                if (i < _.len) {
                    _.regs[i] = _.regs[_.len];
                    _.poll_fds[i] = _.poll_fds[_.len];
                }
                return true;
            }
        }
        return false;
    }

    pub fn poll(*_, timeout_ms: i32): usize {
        if (_.len == 0) return 0;

        let slice = _.poll_fds[0.._.len];
        for (i of 0.._.len) {
            slice[i].revents = 0;
        }

        const rc = std.os.linux.poll(slice.ptr, slice.len, timeout_ms);
        const ready_count: usize = if (@as(isize, @bitCast(rc)) < 0) 0 else rc;
        if (ready_count == 0) return 0;

        let dispatched: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            const revents = _.poll_fds[i].revents;
            if (revents != 0) {
                let flags: u32 = 0;
                if ((revents & std.os.linux.POLL.IN) != 0) flags |= PollEvents.Readable;
                if ((revents & std.os.linux.POLL.OUT) != 0) flags |= PollEvents.Writable;
                if ((revents & std.os.linux.POLL.ERR) != 0) flags |= PollEvents.Error;
                if ((revents & std.os.linux.POLL.HUP) != 0) flags |= PollEvents.Hup;

                const reg = _.regs[i];
                reg.callback(reg.fd, flags, reg.ctx);
                dispatched += 1;
            }
            i += 1;
        }
        return dispatched;
    }
}
