// Source: UNVERIFIED (origin to be confirmed: Bun / Zig std / custom)
/**
 * CP HTTP Response (cap:http)
 */
pub class Response {
    pub status: i32;
    pub body: string;
    pub contentType: string;

    pub fn init(*_, body: string) {
        _.status = 200;
        _.body = body;
        _.contentType = "text/plain";
    }

    pub fn text(): string {
        return _.body;
    }

    pub fn json(): std.json.Value! {
        return JSON.parse(_.body);
    }

    pub fn ok(): bool {
        if (_.status >= 200) {
            if (_.status < 300) {
                return true;
            }
        }
        return false;
    }
}
