// Source: Absorbed from /data/dev/bun/src/sql_jsc/mysql/MySQLRequestQueue.cp | Synced: 2026-11-23T02:05:43Z
// Pure-CP zero-allocation MySQL query pipeline & prepared-statement request queue.
// Tracks pipelined vs non-pipelinable in-flight counts, prepare barriers, ready-for-query
// state, head-of-line completed-entry pruning, and bulk cancellation on disconnect.

/// 零堆分配环形请求队列固定容量（槽位数）
pub const MYSQL_REQ_QUEUE_CAPACITY: usize = 64;

/// `MySqlRequestQueue` 调度与容量错误集
pub err MySqlRequestQueueError {
    QueueFull,
    RequestNotFound,
    InvalidStateTransition,
    ConnectionNotWritable,
}

/// 单条 MySQL 查询请求生命周期状态
pub enum MySqlRequestState {
    Pending,
    Preparing,
    Prepared,
    Running,
    Completed,
    Failed,
}

/// 队列中单条 MySQL 请求元数据条目
pub struct MySqlRequestEntry {
    id: u64 = 0,
    state: MySqlRequestState = MySqlRequestState.Pending,
    pipelinable: bool = false,
    pipelined: bool = false,

    /// 是否已处于终态（`Completed` 或 `Failed`）
    fn isCompleted(): bool {
        return _.state == MySqlRequestState.Completed || _.state == MySqlRequestState.Failed;
    }

    /// 是否正在等待 `COM_STMT_PREPARE_OK`
    fn isBeingPrepared(): bool {
        return _.state == MySqlRequestState.Preparing;
    }

    /// 是否正在执行中
    fn isRunning(): bool {
        return _.state == MySqlRequestState.Running;
    }
}

/// 零堆分配 MySQL 请求流水线调度队列（对应 Bun `MySQLRequestQueue`）。
pub struct MySqlRequestQueue {
    entries: [MYSQL_REQ_QUEUE_CAPACITY]MySqlRequestEntry = [MySqlRequestEntry{}; MYSQL_REQ_QUEUE_CAPACITY],
    head: usize = 0,
    count: usize = 0,
    pipelined_requests: u32 = 0,
    nonpipelinable_requests: u32 = 0,
    waiting_to_prepare: bool = false,
    is_ready_for_query: bool = true,
    auto_pipelining_enabled: bool = true,

    /// 初始化默认开启自动流水线的请求队列
    static fn init(): MySqlRequestQueue {
        return MySqlRequestQueue{};
    }

    /// 按指定自动流水线开关初始化请求队列
    static fn initWithPipelining(enabled: bool): MySqlRequestQueue {
        return MySqlRequestQueue{
            auto_pipelining_enabled: enabled,
        };
    }

    fn slotIndex(offset: usize): usize {
        return (_.head + offset) % MYSQL_REQ_QUEUE_CAPACITY;
    }

    fn findOffsetById(id: u64): usize? {
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[_.slotIndex(i)].id == id) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// 队列是否为空
    fn isEmpty(): bool {
        return _.count == 0;
    }

    /// 队列是否已满
    fn isFull(): bool {
        return _.count >= MYSQL_REQ_QUEUE_CAPACITY;
    }

    /// 当前队列中未移除的请求总数
    fn len(): usize {
        return _.count;
    }

    /// 队列固定容量
    fn capacity(): usize {
        return MYSQL_REQ_QUEUE_CAPACITY;
    }

    /// 当前是否允许执行独占（非流水线）查询（对齐 Bun `canExecuteQuery`）
    fn canExecuteQuery(connection_writable: bool): bool {
        return connection_writable &&
            _.is_ready_for_query &&
            _.nonpipelinable_requests == 0 &&
            _.pipelined_requests == 0;
    }

    /// 当前是否允许发起 `COM_STMT_PREPARE`（对齐 Bun `canPrepareQuery`）
    fn canPrepareQuery(connection_writable: bool): bool {
        return connection_writable &&
            _.is_ready_for_query &&
            !_.waiting_to_prepare &&
            _.pipelined_requests == 0;
    }

    /// 当前是否允许继续流水线并发派发后续查询（对齐 Bun `canPipeline`）
    fn canPipeline(connection_writable: bool): bool {
        if (!_.auto_pipelining_enabled) {
            return false;
        }
        return _.is_ready_for_query &&
            _.nonpipelinable_requests == 0 &&
            !_.waiting_to_prepare &&
            connection_writable;
    }

    /// 连接就绪回调：标记服务端已准备好接收新查询
    fn markAsReadyForQuery(*_) {
        _.is_ready_for_query = true;
    }

    /// 向队尾入队一条新的 `Pending` 请求，返回当前队列长度
    fn enqueue(*_, id: u64, pipelinable: bool): MySqlRequestQueueError!usize {
        if (_.isFull()) {
            return MySqlRequestQueueError.QueueFull;
        }
        const idx = _.slotIndex(_.count);
        _.entries[idx] = MySqlRequestEntry{
            id: id,
            state: MySqlRequestState.Pending,
            pipelinable: pipelinable,
            pipelined: false,
        };
        _.count += 1;
        return _.count;
    }

    /// 查看队首请求（若队列为空返回 `null`）
    fn current(): MySqlRequestEntry? {
        if (_.count == 0) {
            return null;
        }
        return _.entries[_.head];
    }

    /// 按相对队首偏移量查看请求条目
    fn peekAt(offset: usize): MySqlRequestEntry? {
        if (offset >= _.count) {
            return null;
        }
        return _.entries[_.slotIndex(offset)];
    }

    /// 启动指定请求的 `COM_STMT_PREPARE` 阶段，建立 prepare 屏障
    fn startPrepare(*_, id: u64, connection_writable: bool): MySqlRequestQueueError!bool {
        if (!connection_writable) {
            return MySqlRequestQueueError.ConnectionNotWritable;
        }
        if (!_.canPrepareQuery(connection_writable)) {
            return MySqlRequestQueueError.InvalidStateTransition;
        }
        if (off := _.findOffsetById(id)) {
            const idx = _.slotIndex(off);
            if (_.entries[idx].state != MySqlRequestState.Pending) {
                return MySqlRequestQueueError.InvalidStateTransition;
            }
            _.entries[idx].state = MySqlRequestState.Preparing;
            _.is_ready_for_query = false;
            _.waiting_to_prepare = true;
            return true;
        }
        return MySqlRequestQueueError.RequestNotFound;
    }

    /// 收到 `COM_STMT_PREPARE_OK`：解除 prepare 屏障并将队首 `Preparing` 请求推进至 `Prepared`
    fn markAsPrepared(*_): bool {
        _.waiting_to_prepare = false;
        if (_.count == 0) {
            return false;
        }
        if (_.entries[_.head].state == MySqlRequestState.Preparing) {
            _.entries[_.head].state = MySqlRequestState.Prepared;
            return true;
        }
        return false;
    }

    /// 启动指定请求执行（允许从 `Pending` 或 `Prepared` 进入 `Running`），自动维护流水线计数
    fn startRun(*_, id: u64, connection_writable: bool): MySqlRequestQueueError!bool {
        if (!connection_writable) {
            return MySqlRequestQueueError.ConnectionNotWritable;
        }
        if (_.waiting_to_prepare || _.nonpipelinable_requests > 0) {
            return MySqlRequestQueueError.InvalidStateTransition;
        }
        if (off := _.findOffsetById(id)) {
            const idx = _.slotIndex(off);
            const st = _.entries[idx].state;
            if (st != MySqlRequestState.Pending && st != MySqlRequestState.Prepared) {
                return MySqlRequestQueueError.InvalidStateTransition;
            }
            const use_pipeline = _.auto_pipelining_enabled && _.entries[idx].pipelinable;
            if (!use_pipeline && _.pipelined_requests > 0) {
                return MySqlRequestQueueError.InvalidStateTransition;
            }
            _.entries[idx].state = MySqlRequestState.Running;
            _.entries[idx].pipelined = use_pipeline;
            _.is_ready_for_query = false;
            if (use_pipeline) {
                _.pipelined_requests += 1;
            } else {
                _.nonpipelinable_requests += 1;
            }
            return true;
        }
        return MySqlRequestQueueError.RequestNotFound;
    }

    /// 清理队首所有已处于终态（`Completed`/`Failed`）的请求条目（对应 Bun `AdvanceCleanupGuard`）
    fn pruneCompletedHead(*_): usize {
        let removed: usize = 0;
        while (_.count > 0 && _.entries[_.head].isCompleted()) {
            _.head = (_.head + 1) % MYSQL_REQ_QUEUE_CAPACITY;
            _.count -= 1;
            removed += 1;
        }
        return removed;
    }

    /// 标记队首请求完成（对齐 Bun `markCurrentRequestAsFinished`）并清理队首终态项
    fn finishCurrent(*_): MySqlRequestEntry? {
        if (_.count == 0) {
            return null;
        }
        _.waiting_to_prepare = false;
        let entry = _.entries[_.head];
        if (entry.isBeingPrepared()) {
            _.entries[_.head].state = MySqlRequestState.Prepared;
            return _.entries[_.head];
        }
        if (entry.isRunning()) {
            if (entry.pipelined && _.pipelined_requests > 0) {
                _.pipelined_requests -= 1;
            } else if (!entry.pipelined && _.nonpipelinable_requests > 0) {
                _.nonpipelinable_requests -= 1;
            }
        }
        _.entries[_.head].state = MySqlRequestState.Completed;
        entry.state = MySqlRequestState.Completed;
        * = _.pruneCompletedHead();
        return entry;
    }

    /// 标记队首请求失败并回收其在途计数与队首槽位
    fn failCurrent(*_): MySqlRequestEntry? {
        if (_.count == 0) {
            return null;
        }
        let entry = _.entries[_.head];
        if (entry.isBeingPrepared()) {
            _.waiting_to_prepare = false;
        } else if (entry.isRunning()) {
            if (entry.pipelined && _.pipelined_requests > 0) {
                _.pipelined_requests -= 1;
            } else if (!entry.pipelined && _.nonpipelinable_requests > 0) {
                _.nonpipelinable_requests -= 1;
            }
        }
        _.entries[_.head].state = MySqlRequestState.Failed;
        entry.state = MySqlRequestState.Failed;
        * = _.pruneCompletedHead();
        return entry;
    }

    /// 计算当前连接状态下下一条可派发的请求 ID（严格遵循 Bun `advance` 流水线与 prepare 屏障语义）
    fn nextDispatchableId(connection_writable: bool): u64? {
        if (!connection_writable || _.waiting_to_prepare || _.nonpipelinable_requests > 0) {
            return null;
        }
        let offset: usize = 0;
        while (offset < _.count) {
            const entry = _.entries[_.slotIndex(offset)];
            if (entry.isCompleted()) {
                offset += 1;
                continue;
            }
            if (entry.isBeingPrepared()) {
                return null;
            }
            if (entry.isRunning()) {
                offset += 1;
                continue;
            }
            // Pending 或 Prepared：若已有在途流水线请求，仅当开启自动流水线且当前条目可流水线时允许派发
            if (_.pipelined_requests > 0) {
                if (!_.auto_pipelining_enabled || !entry.pipelinable) {
                    return null;
                }
            }
            return entry.id;
        }
        return null;
    }

    /// 断开连接时清空全部挂起/在途请求并重置计数（对齐 Bun `clean`），返回被取消的未完成请求数
    fn cancelAll(*_): usize {
        let cancelled: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            const idx = _.slotIndex(i);
            if (!_.entries[idx].isCompleted()) {
                _.entries[idx].state = MySqlRequestState.Failed;
                cancelled += 1;
            }
            i += 1;
        }
        _.head = 0;
        _.count = 0;
        _.pipelined_requests = 0;
        _.nonpipelinable_requests = 0;
        _.waiting_to_prepare = false;
        return cancelled;
    }
}
