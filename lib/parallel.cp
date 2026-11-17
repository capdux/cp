// Source: Custom (CP native async/parallel runtime, per CHANGELOG)
/**
 * CP Parallel Computation and Task Orchestration (cap:parallel)
 * 专为单机多核多线程并行计算与高并发任务汇聚设计
 */

pub class Task {
    ptr: *!;

    fn init(*_, ptr: *!) {
        _.ptr = ptr;
    }
}

/**
 * 派发并发异步任务至调度线程池（配合 `@parallel` 多核 Worker 线程池并行执行）。
 * 立即返回轻量级 Task 任务句柄。
 * 对于重度 CPU 密集型或同步阻塞计算，可使用 `spawn.cpu(() => { ... })` 派发至专用计算线程。
 */
pub fn spawn(task_fn: auto): *! {
    return @task_spawn(task_fn);
}

/**
 * 并行汇聚多个异步任务。
 * 阻塞挂起当前协程，待所有任务全部就绪后返回元组结果。
 * 示例: let [r1, r2] = await join(t1, t2);
 */
pub async fn join(t1: auto, t2: auto = null, t3: auto = null, t4: auto = null): *! {
    return await @task_join(t1, t2, t3, t4);
}
