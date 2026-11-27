// Type definitions for CP

import { strEq } from "cap:ascii";
export class RouteParam {
    key: string,
    value: string,
}

export class RouteMatch {
    matched: bool,
    handler_id: u32,
    params: [MAX_PARAMS]RouteParam,
    param_count: usize,
    fn param(key: string): string?;
}

export fn emptyParam(): RouteParam;
export fn noMatch(): RouteMatch;
export enum SegKind {
    Static,
    Param,
    CatchAll
}

export class RouteSeg {
    kind: SegKind,
    text: string,
}

export class RouteEntry {
    active: bool,
    handler_id: u32,
    segs: [MAX_SEGS]RouteSeg,
    seg_count: usize,
}

export fn emptySeg(): RouteSeg;
export fn emptyEntry(): RouteEntry;
export fn segPriority(kind: SegKind): u8;
export fn parseRoutePattern(pattern: string, handler_id: u32): RouteEntry?;
export fn routesConflict(a: RouteEntry, b: RouteEntry): bool;
export fn routeBetterThan(cand: RouteEntry, best: RouteEntry): bool;
export fn matchEntry(entry: RouteEntry, path: string): RouteMatch?;
export class Router {
    routes: [MAX_ROUTES]RouteEntry,
    count: usize,
    fn init(): Router;
    fn insert(_: auto, pattern: string, handler_id: u32): bool;
    fn at(path: string): RouteMatch;
    fn routeCount(): usize;
}

