// Source: cp/lib/health/index.cp (Unified Exports for cap:health)
// Pure CP zero-C-dependency health check probe aggregator and graceful shutdown coordinator.

export {
    HealthError,
    ProbeKind,
    HealthStatus,
    CheckOutcome,
    ProbeReport,
    CheckFn,
    ShutdownHookFn,
    HealthRegistry,
    ShutdownCoordinator,
} from "./probe.cp";
