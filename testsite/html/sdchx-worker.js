"use strict";

let XMLHttpRequest = function() {};
XMLHttpRequest.prototype = {
  open: function() {},
  send: function() {},
};

let Module = {};
importScripts("./vcdiff.js");

const CACHE_NAME = "sdchx-cache";
const DICTIONARIES_CACHE_NAME = "sdchx-cache";

class Dictionary {
  constructor(response) {
    console.log("Dictionary ctor");

    for (let h of response.headers) {
      console.log('Header', h);
      if (h[0] === "sdchx-server-id") {
        this.serverId = h[1];
        this.clientId = h[1].substr(0, 8);
      } else if (h[0] === "sdchx-algo") {
        this.algo = h[1];
      } else if (h[0] === "sdchx-tag") {
        this.tag = h[1];
      } else if (h[0] === "cache-control") {
        let m = h[1].match(/max-age=(\d+)/);
        if (m) {
          this.maxAge = m[1];
        }
      }
    }

    response.clone().arrayBuffer().then(body => {
      console.log("Create VCDiffStreamingDecoder", body.byteLength);
      this.dict = new Uint8Array(body);
    });

    // TODO(bacek) Check Server-Id vs sha256(response.body) to detect corruptions
    console.log(this);
  }

  decode(encoded) {
    let decoder = new Module.VCDiffStreamingDecoder(this.dict);
    let res = decoder.decodeChunk(encoded);
    decoder.finish();
    return res;
  }
}

class DictionaryFactory {
  constructor() {
    console.log("DictionaryFactory ctor");
    this.cache = caches.open(DICTIONARIES_CACHE_NAME);
    this.dictionaries = {};
    this.inFlight = {};
    this.dictById = {};
  }

  onLink(link) {
    console.log('onLink', link);
    if (this.dictionaries[link] || this.inFlight[link]) {
      console.log("DF Skipping " + link);
    } else {
      // Just call fetch. processResponse will do proper things
      this.inFlight[link] = doFetch(new Request(link)).then(resp => {
        delete this.inFlight[link];
        return resp;
      });
    }
  }

  onDictionary(response) {
    if (!this.dictionaries[response.url]) {
      let d = new Dictionary(response);
      this.dictionaries[response.url] = d;
      this.dictById[d.serverId] = d;
    }
  }

  getAvailDictionariesHeader() {
    console.log("getAvailDictionariesHeader");
    if (!this.dictionaries)
      return null;

    var ids = Object.keys(this.dictionaries).map(k => {
      return this.dictionaries[k].clientId;
    });
    console.log(ids);
    if (ids.length > 0) {
      return ["SDCHx-Avail-Dictionary", ids.join(", ")];
    }

    return null;
  }

  getDictionary(serverId) {
    return this.dictById[serverId];
  }
}

let dictionaryFactory = new DictionaryFactory();

/**
 * Process response from network
 */
function processResponse(response) {
  console.log("Processing response", response);
  extractDictionaryLinks(response);
  if (response.headers.has("SDCHx-Server-Id")) {
    dictionaryFactory.onDictionary(response);
  }
  return response;
}

/**
 * Extract Link headers from response and fetch advertised dictionaries
 */
function extractDictionaryLinks(response) {
  console.log("extractDictionaryLinks");
  let links = response.headers.getAll("Link");
  console.log("Links", links);
  let urls = links.map(v => {
    let m = v.match(/<(\S+)>.*rel="sdchx-dictionary"/);
    return m[1];
  })
  .filter(v => {
    return v !== null;
  });

  console.log("final", urls);
  urls.forEach(link => {
    dictionaryFactory.onLink(link);
  });
}

/**
 * Decode VCDiff encoded content.
 * Browser will unzip it for us.
 */
function maybeDecodeContent(response) {
  console.log("maybeDecodeContent");
  if (response.status === 242) {
    console.log("GOT ENCODED CONTENT!!!");
    let id = response.headers.get("sdchx-used-dictionary-id");
    let d = dictionaryFactory.getDictionary(id);
    if (d === null) {
      throw Error("Unknow dictionary " + id);
    }

    return response.arrayBuffer().then(body => {
      console.log("Encoded", body.byteLength);
      let decoded = d.decode(body);
      console.log("Decoded", decoded);

      let init = {
        status: 200,
        // TODO(bacek) Copy appropriate bits from original
      };
      return new Response(decoded, init);
    });
  }
  return response;
}

/**
 * Cache result from network request
 */
function cacheIt(request, response) {
  caches.open(CACHE_NAME).then(cache => {
    console.log("caching", request.url);
    cache.put(request, response.clone());
  });
  return response;
}

/**
 * Try to fetch resource from cache. Fallback to network fetch in case of error
 */
function fetchFromCache(request) {
  return caches.open(CACHE_NAME).then(cache => {
    return cache.match(request).then(response => {
      if (response) {
        console.log('Return cached', request.url);
        return response;
      }

      console.log('Do network fetch');
      return fetch(request).then(resp => {
        return cacheIt(request, resp);
      });
    });
  });
}

/**
 * Actually fetch url.
 */
function doFetch(request) {
  console.log("Fetching ", request.url);
  return fetchFromCache(createRequest(request))
      .then(processResponse) // processResponse should after cacheIt() inside
                             // fetchFromCache. But for
                             // now it simplify development to be able to
                             // process cached results
      .then(maybeDecodeContent);
}

/**
 * Create SDCHx-enabled GET request for given url
 */
function createRequest(original) {
  let options = {
    headers: {
      "X-Accept-Encoding": "sdchx"
    }
  };
  for (let h of original.headers) {
    options.headers[h[0]] = h[1];
  }

  let avail = dictionaryFactory.getAvailDictionariesHeader();
  if (avail !== null) {
    console.log("Adding header", avail);
    options.headers[avail[0]] = avail[1];
  }

  return new Request(original.url, options);
}

self.addEventListener("install", event => {
  // Message to simply show the lifecycle flow
  console.log("[install] Kicking off service worker registration!");
  self.skipWaiting();
});

self.addEventListener("fetch", event => {
  if (event.request.method === "GET") {
    console.log("Handling ", event.request.url);
    event.respondWith(doFetch(event.request));
  }
});

self.addEventListener("activate", event => {
  // Claim the service work for this client, forcing `controllerchange` event
  console.log("[activate] Claiming this service worker!");
  event.waitUntil(self.clients.claim());
});
