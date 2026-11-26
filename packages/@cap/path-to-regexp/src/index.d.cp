// Type definitions for CP

import { eqAscii } from "../../unicase/src/index.cp";
export fn ptrStrEq(a: string, b: string): bool;
export fn ptrSegEq(a: string, b: string, case_sensitive: bool): bool;
export fn ptrTrimSlashes(s: string): string;
export fn ptrFindSegEnd(s: string, pos: usize): usize;
export fn ptrSkipSlashes(s: string, pos: usize): usize;
export fn ptrWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn ptrWriteSlice(out: []u8, pos: usize, s: string): usize;
export fn ptrFindParam(params: &[]RouteParam, name: string): string?;
export class RouteParam {
    name: string,
    value: string,
    fn new(name: string, value: string): RouteParam;
}

export class RouteMatch {
    matched: bool,
    params: [16]RouteParam,
    param_count: usize,
    fn noMatch(): RouteMatch;
    fn getParam(name: string): string?;
}

export fn matchPath(pattern: string, path: string, case_sensitive: bool): RouteMatch;
export fn compilePath(pattern: string, params: &[]RouteParam, out: []u8): usize;
export fn countRouteParams(pattern: string): usize;
