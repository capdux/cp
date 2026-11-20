// Source: cp/lib/expr/index.cp (Unified Exports for cap:expr)
// Unified Public Exports for Safe Deterministic Expression & Formula Evaluator (cap:expr & cap.expr)

export {
    DEFAULT_MAX_DEPTH,
    ExprError,
    ExprValue,
    ExprContext,
    CompiledExpr,
    compile,
    compileWithMaxDepth,
    eval,
} from "./expr.cp";
