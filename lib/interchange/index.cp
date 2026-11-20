// Source: cp/lib/interchange/index.cp (Unified Exports for cap:interchange)
// Industrial-grade pure CP unified data interchange library (JSON / JSONC / JSON5 / TOML / YAML).

export {
    INTERCHANGE_MAX_DEPTH,
    InterchangeFormat,
    InterchangeError,
    InterchangeKind,
    InterchangeEntry,
    InterchangeValue,
} from "./value.cp";

export {
    Json5Codec,
} from "./json5.cp";

export {
    TomlYamlBridge,
} from "./bridge.cp";

export {
    Interchange,
    detectFormat,
    parse,
    stringify,
} from "./interchange.cp";
