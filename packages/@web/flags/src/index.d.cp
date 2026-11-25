// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class FlagsConfig {
    status_path: string,
    user_header: string,
}

export class FeatureFlag {
    name: string,
    enabled: bool,
    rollout_pct: u8,
    allow_users: string,
}

export fn flgStrEql(a: string, b: string): bool;
export fn flgInList(list: string, target: string): bool;
export fn flagBucket(flag_name: string, user_key: string): u8;
export fn flagReset();
export fn flagDefine(name: string, enabled: bool, rollout_pct: u8, allow_users: string): bool;
export fn flagEval(flag_name: string, user_key: string): bool;
export fn flgWriteRaw(dst: []u8, pos: &mut usize, s: string): bool;
export fn flgWriteU8(dst: []u8, pos: &mut usize, v: u8): bool;
export fn handleFlagsStatus(ctx: &mut Context): Response;
export fn createFlagsPlugin(cfg: FlagsConfig): Plugin;
export fn registerFlags(app: &mut App, cfg: FlagsConfig): &mut App;
