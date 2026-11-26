// Type definitions for CP

export class ParamPair {
    name: string,
    value: string,
}

export class RouteMatch {
    matched: bool,
    params: [MAX_PARAMS]ParamPair,
    param_count: usize,
    fn miss(): RouteMatch;
    fn okEmpty(): RouteMatch;
    fn pushParam(_: auto, name: string, value: string);
    fn get(name: string): string?;
    fn has(name: string): bool;
}

export fn strEq(a: string, b: string): bool;
export fn nextSlash(s: string, start: usize): usize;
export fn stripQueryAndTrailingSlash(path: string): string;
export fn stripTrailingSlashOnly(s: string): string;
export fn matchRoute(pattern_raw: string, path_raw: string): RouteMatch;
export fn isRouteMatch(pattern: string, path: string): bool;
export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn findParam(params: []ParamPair, name: string): string?;
export fn buildRoute(pattern_raw: string, params: []ParamPair, out: []u8): string?;
