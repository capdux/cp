// Type definitions for CP

import { eqIgnoreCase, isDigit } from "cap:ascii";
import { indexOfBytes } from "cap:bin";
export enum DeviceKind {
    Desktop = 0,
    Mobile = 1,
    Tablet = 2,
    Bot = 3,
    Cli = 4,
    Unknown = 5
}

export enum BrowserKind {
    Chrome = 0,
    Edge = 1,
    Firefox = 2,
    Safari = 3,
    Opera = 4,
    Samsung = 5,
    Curl = 6,
    Bun = 7,
    Cap = 8,
    Bot = 9,
    Unknown = 10
}

export enum OsKind {
    Windows = 0,
    MacOS = 1,
    IOS = 2,
    Android = 3,
    Linux = 4,
    HarmonyOS = 5,
    ChromeOS = 6,
    Unknown = 7
}

export class UserAgent {
    browser: BrowserKind,
    browser_name: string,
    browser_version: string,
    os: OsKind,
    os_name: string,
    os_version: string,
    device: DeviceKind,
    engine_name: string,
    is_bot: bool,
    is_mobile: bool,
    is_ai_bot: bool,
    bot_name: string,
}

export fn findSubstrCI(haystack: string, needle: string): usize?;
export fn contains(haystack: string, needle: string): bool;
export fn containsCI(haystack: string, needle: string): bool;
export fn isVersionChar(c: u8): bool;
export fn extractVersionAfter(ua: string, prefix: string): string;
export fn extractVersionAfterCI(ua: string, prefix: string): string;
export class BotMatch {
    matched: bool,
    is_ai: bool,
    name: string,
    version: string,
}

export fn matchAiBot(ua: string): BotMatch;
export fn matchAnyBot(ua: string): BotMatch;
export fn parseOs(ua: string, out: &mut UserAgent);
export fn isGenericCli(ua: string): bool;
export fn parseBrowserAndDevice(ua: string, out: &mut UserAgent);
export fn parseUserAgent(ua: string): UserAgent;
export fn isBot(ua: string): bool;
export fn isAiBot(ua: string): bool;
export fn isMobile(ua: string): bool;
