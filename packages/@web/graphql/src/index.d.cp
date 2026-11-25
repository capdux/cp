// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class GraphQLConfig {
    endpoint_path: string,
    enable_playground: bool,
    max_depth: usize,
}

export class GqlResolverEntry {
    name: string,
    resolver: ?GraphQLResolverFn,
}

export fn gqlBytesEql(a: string, b: string): bool;
export fn gqlWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
export fn gqlWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn gqlHexLower(n: u8): u8;
export fn gqlWriteJsonEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn gqlIsWs(c: u8): bool;
export fn gqlIsIdentStart(c: u8): bool;
export fn gqlIsIdentCont(c: u8): bool;
export fn gqlSkipWsAndComments(s: string, start: usize): usize;
export fn gqlCheckQueryDepth(query: string, max_depth: usize): bool;
export fn gqlRegisterResolver(field_name: string, resolver: GraphQLResolverFn): bool;
export fn gqlResetResolvers();
export fn gqlFindResolver(field_name: string): ?GraphQLResolverFn;
export fn gqlSkipJsonWs(s: string, start: usize): usize;
export fn gqlExtractJsonQuery(body: string, out_buf: []u8): ?usize;
export fn gqlLooksLikeJsonValue(val: string): bool;
export fn gqlSkipBalanced(s: string, start: usize, open_ch: u8, close_ch: u8): usize;
export fn gqlRenderPlaygroundHtml(endpoint: string, out_buf: []u8): ?usize;
export fn gqlGetHandler(ctx: &mut Context): Response;
export fn gqlPostHandler(ctx: &mut Context): Response;
export fn registerGraphQL(app: &mut App, cfg: GraphQLConfig): &mut App;
