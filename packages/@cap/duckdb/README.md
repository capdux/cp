<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c++** [`duckdb`](https://github.com/duckdb/duckdb)（`MIT`）（包名 `@cap/duckdb`）。

# `@cap/duckdb`

In-process analytical SQL (OLAP) database engine for CP, powered by **DuckDB v1.2.1** (MIT License) using official prebuilt shared libraries (`sys.duckdb.prebuilt`) with zero local C++ build time.

## Architecture

- **Prebuilt Dynamic Library**: Links directly against DuckDB's official release `libduckdb` (`prebuilt/<os>-<arch>/libduckdb.{so,dylib,dll}`), with automatic runtime `LD_LIBRARY_PATH` / `DYLD_LIBRARY_PATH` injection by `cap run`.
- **Minimal C ABI Shim (`c/duckdb_ffi.c`)**: ~35-line heap-allocated wrapper around `duckdb_result` compiled in ~0.1s into `libduckdb_shim.a`, guaranteeing 100% ABI safety across platforms without hand-mirroring large C structs in CP.
- **Zero-Leak RAII (`src/index.cp`)**: Provides `DuckDatabase`, `DuckConn`, and `DuckResult` structs with deterministic `~init()` cleanup.

## Downloading Official Prebuilt Binaries

Official prebuilt shared libraries are distributed by DuckDB Releases (`v1.2.1`) and are excluded from Git tracking due to binary size (~60MB):

| Target | Official Release Asset | Destination Path |
| --- | --- | --- |
| `linux-x86_64` | `https://github.com/duckdb/duckdb/releases/download/v1.2.1/libduckdb-linux-amd64.zip` | `prebuilt/linux-x86_64/libduckdb.so` |
| `linux-aarch64` | `https://github.com/duckdb/duckdb/releases/download/v1.2.1/libduckdb-linux-aarch64.zip` | `prebuilt/linux-aarch64/libduckdb.so` |
| `macos-universal` | `https://github.com/duckdb/duckdb/releases/download/v1.2.1/libduckdb-osx-universal.zip` | `prebuilt/macos-aarch64/libduckdb.dylib` or `prebuilt/macos-x86_64/libduckdb.dylib` |
| `windows-x86_64` | `https://github.com/duckdb/duckdb/releases/download/v1.2.1/libduckdb-windows-amd64.zip` | `prebuilt/windows-x86_64/duckdb.dll` |

Example setup on Linux x86_64:

```bash
curl -sSL -o /tmp/libduckdb.zip https://github.com/duckdb/duckdb/releases/download/v1.2.1/libduckdb-linux-amd64.zip
unzip -o /tmp/libduckdb.zip libduckdb.so -d prebuilt/linux-x86_64/
unzip -o /tmp/libduckdb.zip duckdb.h -d include/
```

## Quick Start

```cp
import { DuckDatabase, libraryVersion } from "@cap/duckdb";
import std from "std";

pub fn main(): void! {
    std.debug.print("DuckDB: {s}\n", .{libraryVersion()});
    let db = try DuckDatabase.open(":memory:");
    let conn = try db.connect();
    let res = try conn.query("SELECT SUM(v), AVG(v), COUNT(*) FROM range(100000) t(v)");
    std.debug.print("sum={} avg={d:.1} cnt={}\n", .{
        res.valueI64(0, 0),
        res.valueF64(1, 0),
        res.valueI64(2, 0),
    });
}
```
