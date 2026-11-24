// Regression test for task-bun-20261124-struct-addr-of:
// User-defined `const Task = struct { ... };` (shadowing built-in heap class name `Task`)
// must NOT be treated as an implicit Zig heap pointer, so `&_.tasks[idx]` preserves `&`
// and mutates the slice element in place rather than copying by value.

const Id = struct {
    raw: usize,
    pub fn get(): usize { return _.raw; }
};

const Server = struct {
    result: u32,
};

const Request = struct {
    code: i32,
};

pub struct Runner {
    tasks: []Server,
    reqs: []Request,

    pub fn startTask(*_, entry_id: Id) {
        let task = &_.tasks[entry_id.get()];
        task.result = 42;
    }

    pub fn updateReq(*_, idx: usize, code: i32) {
        let req = &_.reqs[idx];
        req.code = code;
    }
}

fn main() {
    let task_buf: [2]Server = [Server { result: 0 }, Server { result: 0 }];
    let req_buf: [2]Request = [Request { code: 0 }, Request { code: 0 }];
    let runner = Runner {
        tasks: task_buf[0..],
        reqs: req_buf[0..],
    };
    runner.startTask(Id { raw: 1 });
    runner.updateReq(0, 200);

    if (task_buf[0].result != 0) std.debug.panic("unexpected task[0] mutation", .{});
    if (task_buf[1].result != 42) std.debug.panic("task[1] was copied by value instead of mutated via &", .{});
    if (req_buf[0].code != 200) std.debug.panic("req[0] was copied by value instead of mutated via &", .{});
}
