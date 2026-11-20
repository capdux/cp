// Source: cp/lib/rbac/index.cp (Unified Exports for cap:rbac)
// Pure CP enterprise RBAC & IAM policy evaluation engine with hierarchical role
// inheritance, cycle detection, wildcard permission matching, and explicit-deny-overrides.

export {
    RbacError,
    PolicyEffect,
    PermissionMatcher,
} from "./permission.cp";

export {
    AccessDecision,
    RbacEngine,
} from "./engine.cp";
