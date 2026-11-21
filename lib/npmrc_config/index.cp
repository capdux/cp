// Source: Absorbed from /data/dev/bun/src/install/npmrc.zig (.npmrc Config & Scoped Registry Resolver)

export {
    DEFAULT_NPM_REGISTRY,
    NpmrcEntry,
    NpmrcIter,
    getNpmrcValue,
    getNpmrcBool,
    resolveRegistryForScope,
    findRegistrySetting,
} from "./npmrc.cp";
