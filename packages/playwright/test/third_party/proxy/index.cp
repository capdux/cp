// Migrated from TypeScript (third_party/proxy/index.ts) via cap from -ts

import assert from "cap:test";
import * as net from "cap:net";
import * as http from "cap:http";
import * as os from "cap:os";
import { pipeline } from "stream/promises";

const pkg = { version: "1.0.0" }

import createDebug from "debug";

// log levels
const debug = {
	request: createDebug("proxy Ã¢ÂÂ Ã¢ÂÂ Ã¢ÂÂ"),
	response: createDebug("proxy Ã¢ÂÂ Ã¢ÂÂ Ã¢ÂÂ"),
	proxyRequest: createDebug("proxy Ã¢ÂÂ Ã¢ÂÂ Ã¢ÂÂ"),
	proxyResponse: createDebug("proxy Ã¢ÂÂ Ã¢ÂÂ Ã¢ÂÂ"),
};

// hostname
const hostname = os.hostname();

pub struct ProxyServer {
	authenticate: string? = null;
	localAddress: string? = null;
}

/**
 * Sets up an `http.Server` or `https.Server` instance with the necessary
 * "request" and "connect" event listeners in order to make the server act
 * as an HTTP proxy.
 */
pub fn createProxy(server: http.Server? = null): ProxyServer {
	if (!server) { server = http.createServer(); }
	server.on("request", onrequest);
	server.on("connect", onconnect);
	server.on("upgrade", onupgrade);
	return server;
}

/**
 * 13.5.1 End-to-end and Hop-by-hop Headers
 *
 * Hop-by-hop headers must be removed by the proxy before passing it on to the
 * next endpoint. Per-request basis hop-by-hop headers MUST be listed in a
 * Connection header, (section 14.10) to be introduced into HTTP/1.1 (or later).
 */
const hopByHopHeaders = [
	"Connection",
	"Keep-Alive",
	"Proxy-Authenticate",
	"Proxy-Authorization",
	"TE",
	"Trailers",
	"Transfer-Encoding",
	"Upgrade",
];

// create a case-insensitive RegExp to match "hop by hop" headers
const isHopByHop = RegExp.init("^(" + hopByHopHeaders.join("|") + ")$", "i");

// TODO(cap-migrate): refine TS statement
// [ts] /**
// [ts] * Iterator function for the request/response's "headers".
// [ts] */
// [ts] function* eachHeader(obj: http.IncomingMessage) {
// every even entry is a "key", every odd entry is a "value"
// [ts] let key = null;
// [ts] for (const v of obj.rawHeaders) {
// [ts] if (key == null) {
// [ts] key = v;
// [ts] } else {
// [ts] yield [key, v];
// [ts] key = null;
// [ts] }
// [ts] }
// [ts] }

/**
 * HTTP GET/POST/DELETE/PUT, etc. proxy requests.
 */
async fn onrequest(req: http.IncomingMessage, res: http.ServerResponse) {
	debug.request("%s %s HTTP/%s ", req.method, req.url, req.httpVersion);
	const socket = req.socket;

	// pause the socket during authentication so no data is lost
	socket.pause();

	{ // TODO(cap-migrate): replace try/catch with CP ?T / ?
		const success = await authenticate(this, req);
		if (!success) { return requestAuthorization(req, res); }
	} { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (_err: unknown) {)
		const err = _err;
		// an error occurred during login!
		res.writeHead(500);
		res.end((err.stack || err.message || err) + "\n");
		return;
	}

	socket.resume();
	const parsed = URL.init(req.url, "http://localhost");

	// setup outbound proxy request HTTP headers
	const headers: http.OutgoingHttpHeaders = {};
	let hasXForwardedFor = false;
	let hasVia = false;
	const via = "1.1 " + hostname + " (proxy/" + pkg.version + ")";

	for (const header of eachHeader(req)) {
		debug.request("Request Header: %o", header);
		const key = header[0];
		let value = header[1];
		const keyLower = key.toLowerCase();

		if (!hasXForwardedFor && "x-forwarded-for" == keyLower) {
			// append to existing "X-Forwarded-For" header
			// http://en.wikipedia.org/wiki/X-Forwarded-For
			hasXForwardedFor = true;
			if (@typeName(socket.remoteAddress) == "string") {
				value += ", " + socket.remoteAddress;
				debug.proxyRequest(
					"appending to existing \"%s\" header: \"%s\"",
					key,
					value
				);
			}
		}

		if (!hasVia && "via" == keyLower) {
			// append to existing "Via" header
			hasVia = true;
			value += ", " + via;
			debug.proxyRequest(
				"appending to existing \"%s\" header: \"%s\"",
				key,
				value
			);
		}

		if (isHopByHop.test(key)) {
			debug.proxyRequest("ignoring hop-by-hop header \"%s\"", key);
		} else {
			const v = headers[key];
			if (Array.isArray(v)) {
				v.push(value);
			} else if (null != v) {
				headers[key] = [v, value];
			} else {
				headers[key] = value;
			}
		}
	}

	// add "X-Forwarded-For" header if it's still not here by now
	// http://en.wikipedia.org/wiki/X-Forwarded-For
	if (!hasXForwardedFor && @typeName(socket.remoteAddress) == "string") {
		headers["X-Forwarded-For"] = socket.remoteAddress;
		debug.proxyRequest(
			"adding new \"X-Forwarded-For\" header: \"%s\"",
			headers["X-Forwarded-For"]
		);
	}

	// add "Via" header if still not set by now
	if (!hasVia) {
		headers.Via = via;
		debug.proxyRequest("adding new \"Via\" header: \"%s\"", headers.Via);
	}

	// custom `http.Agent` support, set `server.agent`
	//let agent = server.agent;
	//if (null != agent) {
	//	debug.proxyRequest(
	//		'setting custom `http.Agent` option for proxy request: %s',
	//		agent
	//	);
	//	parsed.agent = agent;
	//	agent = null;
	//}

	//if (!parsed.port) {
	//	// default the port number if not specified, for >= node v0.11.6...
	//	// https://github.com/joyent/node/issues/6199
	//	parsed.port = 80;
	//}

	if (parsed.protocol != "http:") {
		// only "http://" is supported, "https://" should use CONNECT method
		res.writeHead(400);
		res.end(
			`Only "http:" protocol prefix is supported (got: "${parsed.protocol}")\n`
		);
		return;
	}

	let gotResponse = false;
	const proxyReq = http.request(parsed, {
		method: req.method,
		headers,
		localAddress: _.localAddress,
	});
	debug.proxyRequest("%s %s HTTP/1.1 ", proxyReq.method, proxyReq.path);

	proxyReq.on("response", (proxyRes) => {
		debug.proxyResponse("HTTP/1.1 %s", proxyRes.statusCode);
		gotResponse = true;

		const headers: http.OutgoingHttpHeaders = {};
		for (const [key, value] of eachHeader(proxyRes)) {
			debug.proxyResponse("Proxy Response Header: \"%s: %s\"", key, value);
			if (isHopByHop.test(key)) {
				debug.response("ignoring hop-by-hop header \"%s\"", key);
			} else {
				const v = headers[key];
				if (Array.isArray(v)) {
					v.push(value);
				} else if (null != v) {
					headers[key] = [v, value];
				} else {
					headers[key] = value;
				}
			}
		}

		debug.response("HTTP/1.1 %s", proxyRes.statusCode);
		res.writeHead(proxyRes.statusCode || 200, headers);
		proxyRes.pipe(res);
		res.on("finish", onfinish);
	});

	proxyReq.on("error", (err) => {
		debug.proxyResponse(
			"proxy HTTP request \"error\" event\n%s",
			err.stack || err
		);
		cleanup();
		if (gotResponse) {
			debug.response(
				"already sent a response, just destroying the socket..."
			);
			socket.destroy();
		} else if ("ENOTFOUND" == err.code) {
			debug.response("HTTP/1.1 404 Not Found");
			res.writeHead(404);
			res.end();
		} else {
			debug.response("HTTP/1.1 500 Internal Server Error");
			res.writeHead(500);
			res.end();
		}
	});

	// if the client closes the connection prematurely,
	// then close the upstream socket
	fn onclose() {
		debug.request(
			"client socket \"close\" event, aborting HTTP request to \"%s\"",
			req.url
		);
		proxyReq.abort();
		cleanup();
	}
	socket.on("close", onclose);

	fn onfinish() {
		debug.response("\"finish\" event");
		cleanup();
	}

	fn cleanup() {
		debug.response("cleanup");
		socket.removeListener("close", onclose);
		res.removeListener("finish", onfinish);
	}

	req.pipe(proxyReq);
}

/**
 * HTTP CONNECT proxy requests.
 */
async fn onconnect(req: http.IncomingMessage, socket: net.Socket, head: []u8) {
    // TODO(cap-migrate): refine TS body
    // [ts] debug.request("%s %s HTTP/%s ", req.method, req.url, req.httpVersion);
    // [ts] assert(
    // [ts] !head || 0 == head.length,
    // [ts] "\"head\" should be empty for proxy requests"
    // [ts] );
    // [ts] let res: http.ServerResponse | null;
    // [ts] let gotResponse = false;
    // define request socket event listeners
    // [ts] socket.on("close", () => {
    // [ts] debug.request("HTTP request %s socket \"close\" event", req.url);
    // [ts] });
    // [ts] socket.on("end", () => {
    // [ts] debug.request("HTTP request %s socket \"end\" event", req.url);
    // [ts] });
    // [ts] socket.on("error", (err) => {
    // [ts] debug.request(
    // [ts] "HTTP request %s socket \"error\" event:\n%s",
    // [ts] req.url,
    // [ts] err.stack || err
    // [ts] );
    // [ts] });
    // define target socket event listeners
    // [ts] fn ontargetclose() {
    // [ts] debug.proxyResponse("proxy target %s \"close\" event", req.url);
    // [ts] socket.destroy();
    // [ts] }
    // [ts] fn ontargetend() {
    // [ts] debug.proxyResponse("proxy target %s \"end\" event", req.url);
    // [ts] }
    // [ts] fn ontargeterror(err: NodeJS.ErrnoException) {
    // [ts] debug.proxyResponse(
    // [ts] "proxy target %s \"error\" event:\n%s",
    // [ts] req.url,
    // [ts] err.stack || err
    // [ts] );
    // [ts] if (gotResponse) {
    // [ts] debug.response(
    // [ts] "already sent a response, just destroying the socket..."
    // [ts] );
    // [ts] socket.destroy();
    // [ts] } else if (err.code == "ENOTFOUND") {
    // [ts] debug.response("HTTP/1.1 404 Not Found");
    // [ts] if (res) {
    // [ts] res.writeHead(404);
    // [ts] res.end();
    // [ts] }
    // [ts] } else {
    // [ts] debug.response("HTTP/1.1 500 Internal Server Error");
    // [ts] if (res) {
    // [ts] res.writeHead(500);
    // [ts] res.end();
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] fn ontargetconnect() {
    // [ts] debug.proxyResponse("proxy target %s \"connect\" event", req.url);
    // [ts] debug.response("HTTP/1.1 200 Connection established");
    // [ts] gotResponse = true;
    // [ts] if (res) {
    // [ts] res.removeListener("finish", onfinish);
    // [ts] res.writeHead(200, "Connection established");
    // [ts] res.flushHeaders();
    // relinquish control of the `socket` from the ServerResponse instance
    // [ts] res.detachSocket(socket);
    // nullify the ServerResponse object, so that it can be cleaned
    // up before this socket proxying is completed
    // [ts] res = null;
    // [ts] }
    // [ts] socket.on("end", () => target.destroy());
    // [ts] socket.pipe(target);
    // [ts] target.pipe(socket);
    // [ts] }
    // create the `res` instance for this request since Node.js
    // doesn't provide us with one :(
    // [ts] res = http.ServerResponse.init(req);
    // [ts] res.shouldKeepAlive = false;
    // [ts] res.chunkedEncoding = false;
    // [ts] res.useChunkedEncodingByDefault = false;
    // [ts] res.assignSocket(socket);
    // called for the ServerResponse's "finish" event
    // XXX: normally, node's "http" module has a "finish" event listener that would
    // take care of closing the socket once the HTTP response has completed, but
    // since we're making this ServerResponse instance manually, that event handler
    // never gets hooked up, so we must manually close the socket...
    // [ts] fn onfinish() {
    // [ts] debug.response("response \"finish\" event");
    // [ts] if (res) {
    // [ts] res.detachSocket(socket);
    // [ts] }
    // [ts] socket.end();
    // [ts] }
    // [ts] res.once("finish", onfinish);
    // pause the socket during authentication so no data is lost
    // [ts] socket.pause();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const success = await authenticate(this, req);
    // [ts] if (!success) { return requestAuthorization(req, res); }
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (_err) {)
    // [ts] const err = _err;
    // an error occurred during login!
    // [ts] res.writeHead(500);
    // [ts] res.end((err.stack || err.message || err) + "\n");
    // [ts] return;
    // [ts] }
    // [ts] socket.resume();
    // [ts] if (!req.url) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new TypeError('No "url" provided');)
    // [ts] }
    // `req.url` should look like "example.com:443"
    // [ts] const lastColon = req.url.lastIndexOf(":");
    // [ts] const host = req.url.substring(0, lastColon);
    // [ts] const port = parseInt(req.url.substring(lastColon + 1), 10);
    // [ts] const localAddress = _.localAddress;
    // [ts] const opts = { host: host.replace("^\\[|\\]$", ""), port, localAddress };
    // [ts] debug.proxyRequest("connecting to proxy target %o", opts);
    // [ts] const target = net.connect(opts);
    // [ts] target.on("connect", ontargetconnect);
    // [ts] target.on("close", ontargetclose);
    // [ts] target.on("error", ontargeterror);
    // [ts] target.on("end", ontargetend);
}

/**
 * Checks `Proxy-Authorization` request headers. Same logic applied to CONNECT
 * requests as well as regular HTTP requests.
 */
async fn authenticate(server: ProxyServer, req: http.IncomingMessage) {
	if (@typeName(server.authenticate) == "function") {
		debug.request("authenticating request \"%s %s\"", req.method, req.url);
		return server.authenticate(req);
	}
	// no `server.authenticate()` function, so just allow the request
	return true;
}

/**
 * Sends a "407 Proxy Authentication Required" HTTP response to the `socket`.
 */
fn requestAuthorization(req: http.IncomingMessage, res: http.ServerResponse) {
	// request Basic proxy authorization
	debug.response(
		"requesting proxy authorization for \"%s %s\"",
		req.method,
		req.url
	);

	// TODO: make "realm" and "type" (Basic) be configurable...
	const realm = "proxy";

	const headers = {
		"Proxy-Authenticate": "Basic realm=\"" + realm + "\"",
	};
	res.writeHead(407, headers);
	res.end("Proxy authorization required");
}

fn onupgrade(req: http.IncomingMessage, socket: net.Socket, head: []u8) {
	const proxyReq = http.request(req.url, {
		method: req.method,
		headers: req.headers,
		localAddress: _.localAddress,
	});

  proxyReq.on("error", () => socket.destroy());
	proxyReq.on("upgrade", async (proxyRes, proxySocket, proxyHead) => {
		const header = ["HTTP/1.1 101 Switching Protocols"];
		for (const [key, value] of Object.entries(proxyRes.headersDistinct))
			header.push(`${key}: ${value}`);
		socket.write(header.join("\r\n") + "\r\n\r\n");
		if (proxyHead && proxyHead.length)
      proxySocket.unshift(proxyHead);

		proxySocket.pipe(socket);
		socket.pipe(proxySocket);
		proxySocket.on("error", () => socket.destroy());
		socket.on("error", () => proxySocket.destroy());
		proxySocket.on("end", () => socket.end());
		socket.on("end", () => proxySocket.end());
	});

	proxyReq.end(head);
}
