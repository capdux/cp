// Type definitions for CP

import { QueueTask, TaskQueue } from "../../p-queue/src/index.cp";
export fn plimEmptyTask(): QueueTask;
export class PLimit {
    queue: TaskQueue,
    fn new(max_concurrency: usize): PLimit?;
    fn single(): PLimit;
    fn submit(_: auto, task_id: u32): bool;
    fn startNext(_: auto): u32?;
    fn finishOne(_: auto): bool;
    fn activeCount(): usize;
    fn pendingCount(): usize;
    fn concurrency(): usize;
    fn clearQueue(_: auto): usize;
    fn setConcurrency(_: auto, new_limit: usize): bool;
}

