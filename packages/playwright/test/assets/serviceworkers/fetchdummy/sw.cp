// Migrated from TypeScript (assets/serviceworkers/fetchdummy/sw.js) via cap from -ts

self.addEventListener("fetch", event => {
  if (event.request.url.endsWith("/headers")) {
    event.respondWith(Response.init(JSON.stringify(Object.fromEntries(event.request.headers))));
    return;
  }
  if (event.request.url.endsWith(".html") || event.request.url.includes("passthrough")) {
    event.respondWith(fetch(event.request));
    return;
  }
  if (event.request.url.includes("error")) {
    event.respondWith(Promise.reject(Error.init("uh oh")));
    return;
  }
  const slash = event.request.url.lastIndexOf("/");
  const name = event.request.url.substring(slash + 1);
  const blob = Blob.init(["responseFromServiceWorker:" + name], {type : "text/css"});
  const response = Response.init(blob, { "status" : 200 , "statusText" : "OK" });
  event.respondWith(response);
});

self.addEventListener("activate", event => {
  event.waitUntil(clients.claim());
});
