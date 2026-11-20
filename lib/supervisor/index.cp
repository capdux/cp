// Source: cp/lib/supervisor/index.cp (Unified Exports for cap:supervisor)
// Pure CP OTP Supervision Tree State Machine & Distributed Resilience Primitives (cap:supervisor & cap.supervisor).

export {
    SupervisorError,
    RestartStrategy,
    ChildRestartPolicy,
    ExitReason,
    ChildState,
    SupervisionDecisionKind,
    ChildSpec,
    SupervisorConfig,
    SupervisionDecision,
    SupervisorTree,
} from "./strategy.cp";

export {
    BackoffKind,
    BackoffPolicy,
    Bulkhead,
    RetryBudget,
} from "./resilience.cp";
