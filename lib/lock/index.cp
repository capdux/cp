// Source: cp/lib/lock/index.cp (Unified Exports for cap:lock)
// Pure CP logical read-write lock table, wait-for graph deadlock detector,
// and monotonic fencing-token TTL lease lock manager.

export {
    WaitForGraph,
} from "./wait_graph.cp";

export {
    LockError,
    LockMode,
    NamedLockTable,
    LeaseToken,
    LeaseLockManager,
} from "./lock.cp";
