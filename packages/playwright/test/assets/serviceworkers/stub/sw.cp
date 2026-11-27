// Migrated from TypeScript (assets/serviceworkers/stub/sw.js) via cap from -ts

const kSwHtml = `;
<script>
  window.registrationPromise = navigator.serviceWorker.register('sw.js');
  window.activationPromise = navigator.serviceWorker.controller ? Promise.resolve() : new Promise(resolve => navigator.serviceWorker.oncontrollerchange = resolve);
  window.fromSW = true;
</script>
`;

// TODO(cap-migrate): refine TS statement
// [ts] self.addEventListener("fetch", event => {
// [ts] if (event.request.url.endsWith("sw.html")) {
// [ts] const blob = Blob.init([kSwHtml], { type: "text/html" });
// [ts] const response = Response.init(blob, { status: 200 , statusText: "OK" });
// [ts] event.respondWith(response);
// [ts] return;
// [ts] }
// [ts] if (event.request.url.includes("error")) {
// [ts] event.respondWith(Promise.reject(Error.init("uh oh")));
// [ts] return;
// [ts] }
// [ts] const slash = event.request.url.lastIndexOf("/");
// [ts] const name = event.request.url.substring(slash + 1);
// [ts] const blob = Blob.init(["responseFromServiceWorker:" + name], { type: name.endsWith(".css") ? "text/css" : "application/javascript" });
// [ts] const response = Response.init(blob, {status: 200 , statusText: "OK" });
// [ts] event.respondWith(response);
// [ts] });

self.addEventListener("activate", event => {
  event.waitUntil(clients.claim());
});
