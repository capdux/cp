// Type definitions for CP

import { trim, eqIgnoreCase, strEq, startsWith, findByte } from "cap:ascii";
export enum LogLevel {
    Off = 0,
    Error = 1,
    Warn = 2,
    Info = 3,
    Debug = 4,
    Trace = 5
}

export class FilterDirective {
    target: string,
    level: LogLevel,
}

export fn efltLevelRank(lvl: LogLevel): u8;
export fn parseLogLevel(s: string): LogLevel?;
export fn efltMatchesTarget(target: string, directive_target: string): bool;
export class EnvFilter {
    directives: [32]FilterDirective,
    count: usize,
    fn empty(): EnvFilter;
    fn sortDesc(_: auto);
    fn addDirective(_: auto, target: string, level: LogLevel): bool;
    fn parse(spec: string): EnvFilter;
    fn directiveCount(): usize;
    fn getDirective(idx: usize): FilterDirective?;
    fn maxLevelFor(target: string): LogLevel;
    fn enabled(target: string, level: LogLevel): bool;
}

