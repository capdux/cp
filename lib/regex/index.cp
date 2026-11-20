// Source: cp/lib/regex/index.cp (Unified Exports for cap:regex)
// Pure CP zero-C-dependency anti-ReDoS Pike VM / Thompson NFA regular expression engine.

export {
    RegexError,
    Opcode,
    CharClass,
    Inst,
    Program,
    Compiler,
} from "./compiler.cp";

export {
    MatchSpan,
    Captures,
    Regex,
} from "./vm.cp";
