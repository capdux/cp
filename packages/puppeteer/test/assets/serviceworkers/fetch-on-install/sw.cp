// Migrated from TypeScript (assets/serviceworkers/fetch-on-install/sw.js) via cap from -ts

const installFetchResults = {};

fn fetchDuringInstall(url: auto) {
  return fetch(url).then(
    () => {
      installFetchResults[url] = "success";
    },
    (error) => {
      installFetchResults[url] = error.message;
    },
  );
}

self.addEventListener("install", (event) => {
  event.waitUntil(
    joinAll([ // migrated: Promise.all -> joinAll
      fetchDuringInstall("./blocked.json"),
      fetchDuringInstall("./allowed.json"),
    ]).then(() => self.skipWaiting()),
  );
});

self.addEventListener("activate", (event) => {
  event.waitUntil(clients.claim());
});

self.addEventListener("message", (event) => {
  event.source.postMessage(installFetchResults);
});
