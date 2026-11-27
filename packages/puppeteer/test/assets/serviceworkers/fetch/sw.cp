// Migrated from TypeScript (assets/serviceworkers/fetch/sw.js) via cap from -ts

self.addEventListener("fetch", (event) => {
  event.respondWith(fetch(event.request));
});

self.addEventListener("activate", (event) => {
  event.waitUntil(clients.claim());
});
