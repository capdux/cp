import { PicoFormatter } from "../../picocolors/src/index.cp";

pub const LISTR_MAX_TASKS: usize = 32;

pub const TASK_PENDING: u8 = 0;
pub const TASK_RUNNING: u8 = 1;
pub const TASK_COMPLETED: u8 = 2;
pub const TASK_SKIPPED: u8 = 3;
pub const TASK_FAILED: u8 = 4;

pub struct ListrOptions {
    pub concurrent: bool,
    pub exit_on_error: bool,
    pub use_color: bool,

    pub static fn defaults(): ListrOptions {
        return ListrOptions {
            concurrent: false,
            exit_on_error: true,
            use_color: true,
        };
    }
}

pub struct TaskNode {
    pub title: string,
    pub state: u8,
    pub retries_left: u8,
    pub attempts: u8,
    pub skip_reason: string,
    pub err_msg: string,
}

fn lstrPush(out: []u8, pos: usize, s: string): usize {
    if (pos >= out.len || s.len == 0) {
        return pos;
    }
    let avail = out.len - pos;
    let n = if (s.len < avail) { s.len } else { avail };
    @memcpy(out[pos..pos + n], s[0..n]);
    return pos + n;
}

pub struct ListrRunner {
    pub opts: ListrOptions,
    pub tasks: [32]TaskNode,
    pub task_count: usize,
    pub halted: bool,

    pub static fn new(opts: ListrOptions): ListrRunner {
        return ListrRunner {
            opts: opts,
            tasks: undefined,
            task_count: 0,
            halted: false,
        };
    }

    pub fn addTask(*_, title: string, max_retries: u8): usize? {
        if (_.task_count >= LISTR_MAX_TASKS) { return null; }
        let idx = _.task_count;
        _.tasks[idx] = TaskNode {
            title: title,
            state: TASK_PENDING,
            retries_left: max_retries,
            attempts: 0,
            skip_reason: "",
            err_msg: "",
        };
        _.task_count = _.task_count + 1;
        return idx;
    }

    pub fn startTask(*_, idx: usize): bool {
        if (idx >= _.task_count || _.halted) { return false; }
        if (_.tasks[idx].state != TASK_PENDING) { return false; }
        _.tasks[idx].state = TASK_RUNNING;
        _.tasks[idx].attempts = _.tasks[idx].attempts + 1;
        return true;
    }

    pub fn completeTask(*_, idx: usize): bool {
        if (idx >= _.task_count) { return false; }
        _.tasks[idx].state = TASK_COMPLETED;
        return true;
    }

    pub fn skipTask(*_, idx: usize, reason: string): bool {
        if (idx >= _.task_count) { return false; }
        _.tasks[idx].state = TASK_SKIPPED;
        _.tasks[idx].skip_reason = reason;
        return true;
    }

    pub fn failTask(*_, idx: usize, msg: string): bool {
        if (idx >= _.task_count) { return false; }
        if (_.tasks[idx].retries_left > 0) {
            _.tasks[idx].retries_left = _.tasks[idx].retries_left - 1;
            _.tasks[idx].state = TASK_PENDING; // eligible for retry
            _.tasks[idx].err_msg = msg;
            return false; // not terminally failed yet
        }
        _.tasks[idx].state = TASK_FAILED;
        _.tasks[idx].err_msg = msg;
        if (_.opts.exit_on_error) {
            _.halted = true;
        }
        return true; // terminally failed
    }

    pub fn countByState(target: u8): usize {
        let c: usize = 0;
        let i: usize = 0;
        while (i < _.task_count) {
            if (_.tasks[i].state == target) {
                c = c + 1;
            }
            i = i + 1;
        }
        return c;
    }

    pub fn renderTaskLine(idx: usize, out: []u8): usize {
        if (idx >= _.task_count) { return 0; }
        let t = _.tasks[idx];
        let fmt = PicoFormatter.new(_.opts.use_color);
        let badge_buf: [64]u8 = undefined;
        let p: usize = 0;
        if (t.state == TASK_COMPLETED) {
            let bn = fmt.green("[OK]", badge_buf[0..64]);
            p = lstrPush(out, p, badge_buf[0..bn] as string);
        } else if (t.state == TASK_FAILED) {
            let bn = fmt.red("[FAIL]", badge_buf[0..64]);
            p = lstrPush(out, p, badge_buf[0..bn] as string);
        } else if (t.state == TASK_SKIPPED) {
            let bn = fmt.yellow("[SKIP]", badge_buf[0..64]);
            p = lstrPush(out, p, badge_buf[0..bn] as string);
        } else if (t.state == TASK_RUNNING) {
            let bn = fmt.cyan("[RUN]", badge_buf[0..64]);
            p = lstrPush(out, p, badge_buf[0..bn] as string);
        } else {
            let bn = fmt.dim("[WAIT]", badge_buf[0..64]);
            p = lstrPush(out, p, badge_buf[0..bn] as string);
        }
        p = lstrPush(out, p, " ");
        p = lstrPush(out, p, t.title);
        if (t.state == TASK_SKIPPED && t.skip_reason.len > 0) {
            p = lstrPush(out, p, " (");
            p = lstrPush(out, p, t.skip_reason);
            p = lstrPush(out, p, ")");
        }
        return p;
    }
}
