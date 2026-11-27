// Type definitions for CP

import { isDigit, eqIgnoreCase } from "cap:ascii";
import { indexOfBytes } from "cap:bin";
export enum PlatformType {
    Desktop,
    Mobile,
    Tablet,
    Bot,
    Unknown
}

export class ParsedUA {
    browser_name: string,
    browser_version: string,
    os_name: string,
    os_version: string,
    engine_name: string,
    platform: PlatformType,
}

export class BwsPair {
    name: string,
    version: string,
}

export fn bwsIsVersionChar(c: u8): bool;
export fn bwsFindSubstrCI(haystack: string, needle: string): usize?;
export fn bwsContains(haystack: string, needle: string): bool;
export fn bwsContainsCI(haystack: string, needle: string): bool;
export fn bwsExtractVersionAt(ua: string, start: usize): string;
export fn bwsExtractAfter(ua: string, token: string): string;
export fn bwsDetectBrowser(ua: string): BwsPair;
export fn bwsDetectOS(ua: string): BwsPair;
export fn bwsDetectEngine(ua: string): string;
export fn bwsDetectPlatform(ua: string, os_name: string): PlatformType;
export fn parseUserAgent(ua: string): ParsedUA;
