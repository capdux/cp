// Source: Absorbed from /data/dev/bun/src/dotenv/ | Synced: 2026-11-21T16:48:47Z
// CP Standard Library: cap:dotenv

export {
    MAX_ENV_ENTRIES,
    ENV_BUFFER_CAPACITY,
    EnvEntry,
    EnvMap,
    EnvParser,
    parse,
    parseWithOptions,
    expandString,
} from "./parser.cp";

export {
    DotenvOptions,
    DotenvLoader,
    config,
    load,
} from "./loader.cp";

export {
    isValidEnvKey,
    needsDotenvQuotes,
    formatDotenvValue,
    formatDotenvLine,
    unescapeDotenvDouble,
} from "./escape.cp";

export {
    EnvPair,
    lookupEnv,
    expandEnv,
    hasEnvRefs,
} from "./expand.cp";
