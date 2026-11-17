// Source: UNVERIFIED (origin to be confirmed: Bun / Zig std / custom)
/**
 * CP HTTP Request (cap:http)
 */
pub class Request {
    pub method: string;
    pub url: string;
    pub body: string;

    pub fn init(*_, method: string, url: string, body: string) {
        _.method = method;
        _.url = url;
        _.body = body;
    }

    pub fn text(): string {
        return _.body;
    }

    pub fn json(): std.json.Value! {
        return JSON.parse(_.body);
    }
}
