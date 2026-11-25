// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class OAuth2Config {
    client_id: string,
    authorize_url: string,
    token_url: string,
    redirect_uri: string,
    scope: string,
    login_path: string,
    callback_path: string,
}

export class OaStateSlot {
    buf: [OA_MAX_STATE_LEN]u8,
    len: usize,
    active: bool,
}

export fn oaBytesEql(a: string, b: string): bool;
export fn oaWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
export fn oaWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn oaHexUpper(n: u8): u8;
export fn oaHexLower(n: u8): u8;
export fn oaIsUnreserved(c: u8): bool;
export fn oaWriteUrlEncoded(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn oaWriteJsonEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn oaWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;
export fn oaHasQueryMark(s: string): bool;
export fn oauth2BuildAuthorizeUrl(cfg: OAuth2Config, state: string, code_challenge: string, out_buf: []u8): usize?;
export fn oauth2RegisterState(state: string);
export fn oauth2ConsumeState(state: string): bool;
export fn oauth2ResetStates();
export fn oauth2BuildTokenFormBody(cfg: OAuth2Config, code: string, code_verifier: string, out_buf: []u8): usize?;
export fn oaLoginHandler(ctx: &mut Context): Response;
export fn oaCallbackHandler(ctx: &mut Context): Response;
export fn registerOAuth2(app: &mut App, cfg: OAuth2Config): &mut App;
