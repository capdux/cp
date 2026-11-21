// Source: Absorbed from /data/dev/bun/src/bun.cp | Synced: 2026-11-20T21:53:29Z
// ============================================================================
// lib/_cap/index.cp — `cap` Root Hub Documentation & Static Entry
// ============================================================================
// NOTE:
//   When user code writes:
//     import cap from "cap";
//     // or: import c from "cap";
//     // or: const cap = @import("cap");
//   the `cpc` compiler performs compile-time lazy namespace mounting!
//   Only `lib/_cap/core.cp` plus the exact sub-libraries referenced via
//   `<alias>.<ns>` in the reachable module graph are parsed, transpiled, and
//   mounted into the synthesized `cap` hub. Unreferenced standard libraries
//   are never read from disk and never compiled.
//
// Lazily Mounted Sub-Namespaces (authoritative routing table in cpc/src/resolver.rs):
//   strings / string -> string/index.cp
//   sys              -> sys/index.cp
//   io               -> io/index.cp
//   path / paths     -> path/index.cp
//   bits             -> meta/bits.cp
//   meta             -> meta/index.cp
//   logger           -> logger/index.cp
//   http             -> http/index.cp
//   shell            -> shell/index.cp
//   pkg / install    -> pkg/index.cp
//   css              -> css/index.cp
//   md               -> md/index.cp
//   sql              -> sql/index.cp
//   semver / Semver  -> semver/index.cp
//   col / collections / bit_set -> col/index.cp
//   glob             -> glob/index.cp
//   url              -> url/index.cp
//   encoding / base64 -> encoding/index.cp
//   crypto           -> crypto/index.cp
//   hash             -> hash/index.cp
//   wyhash           -> wyhash/index.cp
//   dns              -> dns/index.cp
//   net              -> net/index.cp
//   s3               -> s3/index.cp
//   redis            -> redis/index.cp
//   pack             -> pack/index.cp
//   patch            -> patch/index.cp
//   dotenv           -> dotenv/index.cp
//   conf             -> conf/index.cp
//   watcher          -> watcher/index.cp
//   which            -> which/index.cp
//   elf              -> elf/index.cp
//   ffi              -> ffi/index.cp
//   napi             -> napi/index.cp
//   sourcemap        -> sourcemap/index.cp
//   resolver         -> resolver/index.cp
//   bundler          -> bundler/index.cp
//   aio              -> aio/index.cp
//   platform         -> platform/index.cp
//   safety           -> safety/index.cp
//   security         -> security/index.cp
//   sync             -> sync/index.cp
//   ptr              -> ptr/index.cp
//   event            -> event/index.cp
//   cli              -> cli/index.cp
//   clap             -> clap/index.cp
//   bench            -> bench/index.cp
//   errno            -> errno/index.cp
//   fs               -> fs.cp
//   json             -> json/index.cp
//   html             -> html/index.cp
//   image            -> image/index.cp
//   blob             -> blob/index.cp
//   mem              -> mem/index.cp
//   ws               -> ws/index.cp
//   router           -> router/index.cp
//   simd             -> simd/index.cp
//   sort             -> sort/index.cp
//   bigint           -> bigint/index.cp
//   test             -> test.cp
// ============================================================================

export * from "./core.cp";
