// Origin: Pure-CP Stdlib (Unified Compiler Frontend, Lexers, DSLs, Query Languages, Shaders, IDL & VM Suite: cap:compiler) | Created: 2026-11-22
// ============================================================================
// Pure-CP Zero-C-Dependency Expressions, Rules, SAT Solver, S-Expr, WAT/WGSL/
// GLSL/SPIR-V, WebIDL, Cypher/SPARQL/PromQL, Protobuf/FlatBuffers/FlexBuffers,
// Pike VM Regex, Nginx & Caddyfile DSL Lexers.
// ============================================================================

// --- Expression Evaluation, Rule Engine, SAT Solver & Pike VM Regex ---
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

export {
    SatError,
    SatStatus,
    SatSolution,
    NamedSolution,
    SatSolver,
    ConstraintModel,
} from "./sat.cp";

export {
    RegexError,
    Opcode,
    PrefilterKind,
    Prefilter,
    CharClass,
    Inst,
    Program,
    Compiler,
} from "./regexCompiler.cp";

export {
    MatchSpan,
    Captures,
    Regex,
} from "./regexVm.cp";

// --- Language Frontends, WebAssembly, Shaders & IDL ---
export {
    SexpTokenKind,
    SexpToken,
    SexpIter,
    validateSexp,
    maxSexpDepth,
    formatSexpCompact,
} from "./sexp.cp";

export * from "./wat.cp";

export {
    WgslStage,
    WgslEntryPoint,
    WgslBinding,
    WgslTokenKind,
    WgslToken,
    WgslIter,
    WgslEntryPointIter,
    WgslBindingIter,
    validateWgslDelimiters,
} from "./wgsl.cp";

export {
    GlslVersion,
    GlslQualifier,
    GlslDecl,
    GlslTokenKind,
    GlslToken,
    GlslIter,
    parseGlslVersion,
    GlslDeclIter,
    validateGlslBraces,
} from "./glsl.cp";

export {
    SPV_MAGIC,
    SPV_HEADER_BYTES,
    OP_NOP,
    OP_SOURCE,
    OP_NAME,
    OP_EXT_INST_IMPORT,
    OP_MEMORY_MODEL,
    OP_ENTRY_POINT,
    OP_EXECUTION_MODE,
    OP_CAPABILITY,
    OP_TYPE_VOID,
    OP_FUNCTION,
    OP_FUNCTION_END,
    SpvHeader,
    SpvInst,
    SpvExecutionModel,
    SpvEntryPoint,
    SpvStats,
    isSpv,
    parseSpvHeader,
    SpvInstIter,
    parseSpvEntryPoint,
    summarizeSpv,
} from "./spv.cp";

export {
    WebIdlDeclKind,
    WebIdlDecl,
    WebIdlTokenKind,
    WebIdlToken,
    WebIdlIter,
    WebIdlDeclIter,
    validateWebIdlBraces,
} from "./idl.cp";

// --- Query Languages & Config DSL Lexers ---
export {
    CYPHER_MAX_DEPTH,
    CypherTokenKind,
    CypherClauseKind,
    CypherToken,
    CypherIter,
    CypherParamIter,
    classifyCypherKeyword,
    validateCypherDelimiters,
    isReadOnlyCypher,
} from "./cypher.cp";

export {
    SparqlQueryForm,
    SparqlPrefix,
    SparqlTokenKind,
    SparqlToken,
    SparqlIter,
    SparqlPrefixIter,
    detectSparqlQueryForm,
    isReadOnlySparql,
    validateSparqlDelimiters,
} from "./sparql.cp";

export {
    NginxTokenKind,
    NginxToken,
    NginxIter,
    NginxDirective,
    NginxBlockIter,
    findNginxDirective,
    validateNginxConfig,
} from "./nginx.cp";

export {
    CaddyTokenKind,
    CaddyToken,
    CaddyIter,
    CaddySiteBlock,
    CaddySiteIter,
    validateCaddyfile,
} from "./caddy.cp";

// --- IDL Wire Formats & Schema Binary Parsers ---
export {
    MAX_FIELD_NUMBER,
    DEFAULT_MAX_DEPTH as PROTO_DEFAULT_MAX_DEPTH,
    PROTOBUF_MAX_RECURSION_DEPTH,
    ProtoError,
    WireType,
    zigzagEncode32,
    zigzagDecode32,
    zigzagEncode64,
    zigzagDecode64,
    copyPayloadSimd,
    copyPayloadScalar,
    ProtoField,
    ProtoWriter,
    ProtoReader,
} from "./protobuf.cp";

export {
    DEFAULT_MAX_MESSAGE_BYTES,
    GRPC_FRAME_HEADER_SIZE,
    GrpcError,
    GrpcStatus,
    GrpcFrame,
    GrpcFrameHeader,
    GrpcFrameCodec,
    GrpcFrameStream,
    GrpcTimeout,
} from "./protobufGrpc.cp";

export {
    FB_SIZE_PREFIX_BYTES,
    FB_FILE_ID_OFFSET,
    FB_FILE_ID_LEN,
    FbVector,
    FbTable,
    fbFileIdentifier,
    fbHasIdentifier,
    fbStripSizePrefix,
    fbRootTable,
} from "./flatbuf.cp";

export {
    FlexType,
    FlexBitWidth,
    UnpackedFlexType,
    flexBitWidthBytes,
    unpackFlexType,
    packFlexType,
    FlexRef,
    parseFlexRoot,
} from "./flexbuf.cp";
