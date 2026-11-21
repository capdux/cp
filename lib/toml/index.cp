// Source: cp/lib/toml/index.cp (Unified Exports for cap:toml)
// Industrial-grade pure CP TOML v1.0.0 parser, type-safe DOM, conflict guard & TOML/JSON serializer.

export {
    TomlError,
    TomlKind,
    TomlEntry,
    TomlArray,
    TomlTable,
    TomlValue,
    TomlDoc,
} from "./value.cp";

export {
    TOML_MAX_DEPTH,
    TomlParser,
    TomlSerializer,
} from "./codec.cp";

export {
    isBareTomlKey,
    formatTomlBasicString,
    formatTomlKey,
    unescapeTomlBasic,
} from "./escape.cp";

export {
    TomlTokenKind,
    TomlToken,
    TomlLexer,
} from "./lexer.cp";
