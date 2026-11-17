// Source: Bun src/router/Route.zig (name-matched, needs review)
// lib/router/route.cp
import { ParamMap } from "./param_map";

pub fn RouteMatch(comptime T: type): type {
    return struct {
        handler: T,
        params: ParamMap,
    };
}
