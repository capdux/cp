// Source: Absorbed from /data/dev/bun/src/router/Route.cp | Synced: 2026-11-22T02:31:34Z
// lib/router/route.cp
import { ParamMap } from "./param_map";

pub fn RouteMatch(comptime T: type): type {
    return struct {
        handler: T,
        params: ParamMap,
    };
}
