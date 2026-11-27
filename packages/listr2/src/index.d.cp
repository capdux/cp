// Type definitions for CP

import { PicoFormatter } from "../../picocolors/src/index.cp";
export class ListrOptions {
    concurrent: bool,
    exit_on_error: bool,
    use_color: bool,
    fn defaults(): ListrOptions;
}

export class TaskNode {
    title: string,
    state: u8,
    retries_left: u8,
    attempts: u8,
    skip_reason: string,
    err_msg: string,
}

export fn lstrPush(out: []u8, pos: usize, s: string): usize;
export class ListrRunner {
    opts: ListrOptions,
    tasks: [32]TaskNode,
    task_count: usize,
    halted: bool,
    fn new(opts: ListrOptions): ListrRunner;
    fn addTask(_: auto, title: string, max_retries: u8): usize?;
    fn startTask(_: auto, idx: usize): bool;
    fn completeTask(_: auto, idx: usize): bool;
    fn skipTask(_: auto, idx: usize, reason: string): bool;
    fn failTask(_: auto, idx: usize, msg: string): bool;
    fn countByState(target: u8): usize;
    fn renderTaskLine(idx: usize, out: []u8): usize;
}

