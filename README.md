# HLSProxy

HLSProxy is fast, low-footprint, HTTP proxy server implemented in C++. Integration test suite done in Python.

Its needed in peer-to-peer streaming system as player's first point of contact where Apple's HTTP live streaming protocol is used. 

The proxy function is first step. In the future it may be upgraded to take media segments from peer-to-peer networking layer. For more info about the concept see https://en.wikipedia.org/wiki/Popcorn_Time.

![P2P streaming purpose](http://dzeri.com/streamlabs/proxy_usage.png)

It's not used in production and so far 4 days of work old.

Basic requirements:

- This local server will serve as a relay between VLC and the CDN
- Supporting only HLS media format
- Solution should be as robust as possible
- HLSProxy shall make request to CDN and serve the result to VLC
- Support both relative and absolute URLs in manifest
- The launch procedure:
  1) Launch HLSProxy server on localhost
  2) Then start a network video from VLC pointing to your local server 
- Required events in the log:
  - When a request is intercepted from VLC
  - When the CDN answers and we send back the answer to VLC
  - Make distinction between a manifest and segment for each new incoming request and served response
  - When the server detects the player switched track 

### Demo:

[VLC GUI and integration test suite screen-cast](http://dzeri.com/streamlabs/HLSProxy-operation-demo.mp4)

### The design of the solution:

Data pipeline: `VLC` >> `HLSProxy` >> `CDN` >> `HLSProxy` >> `VLC`

![Pipeline overview](http://dzeri.com/streamlabs/pipeline-diagram.png)

The processing happens in `URL transformation` stage where URI's from CDN's response gets transformed to hit `HLSProxy` instead of CDN.

The integration tests shall use libvlc to play each stream in specified file.

Input file containing test streams shall have single URL per line. Blank lines are allowed.

libVLC test will first try to play stream from CDN directly and then test if playback works through `HLSProxy`.

Tests that fail to play direct CDN URL will yield "INVALID_URL".

Playback time before integration test concludes successful run shall be different for CDN and `HLSProxy`.

### URL transformation rules:

We can call transformed url `encoded-url`.

CDN url is transformed to connect to `HLSProxy` from
	plain-url:   http://`cdn-host:`cdn-port`/`path`?`query`#`fragment`
to
	encoded-url: http://`hlsproxy-host`:`hlsproxy-port`/0~`cdn-host`~`cdn-port`/`path`?`query`#`fragment`

And in case of HTTPS:
	plain-url:   https://`cdn-host:`cdn-port`/`path`?`query`#`fragment`
to
	encoded-url: http://`hlsproxy-host`:`hlsproxy-port`/1~`cdn-host`~`cdn-port`/`path`?`query`#`fragment`

VLC always use HTTP to connect to `HLSProxy` and `HLSProxy` then uses HTTPS to connect to CDN.

Transformation steps:
1) VLC uses `encoded-url` to create request to `HLSProxy`.
2) `HLSProxy` decodes `encoded-url` and get information `cdn-host`, `cdn-port` and whether to use HTTPS or not
3) `HLSProxy` uses `plain-url` to connect to CDN
4) `HLSProxy` streams response from CDN to VLC transforming URLs from playlist from `plain-url` to `encoded-url`
5) VLC completes HTTP transfer and because URLs are transformed next request will also go to `HLSProxy` server

Example:

plain-url:   `https://bitdash-a.akamaihd.net/content/MI201109210084_1/m3u8s/f08e80da-bf1d-4e3d-8899-f0f6155f6efa.m3u8`

encoded-url: `http://127.0.0.1:8080/1~443~bitdash-a.akamaihd.net/content/MI201109210084_1/m3u8s/f08e80da-bf1d-4e3d-8899-f0f6155f6efa.m3u8`

### Internals:

`class HLSProxyServer` accepts new clients and keeps books about connected VLC players. It is possible to direct multiple VLC players on same `HLSProxy` server.

`class HLSProxyServer` implements threading server. Each new connection is processed in new thread.

Cookies are used to distinguish different players among many requests thay may make. VLC player handles cookies, some other player may ignore cookes.

`class HLSClient` take care of VLC request parsing, contacting CDN and parsing CDN's response. 

`HLSClient::run_player_request_parsing` method is where VLC's request processing takes place. Cookie is read here and if not found new place is created in `PlayerActionTracker HLSProxyServer::_player_action_tracker`.

`class PlayerActionTracker` is used for keeping global state about players. This allows track-change to be detected.

`HLSClient::run_cdn_response_parsing` method is where reponse from CDN is streamed to VLC. 

Detecting if content will be HLS playlist can be done in request, in response and in start of the content. Official spec states:
```
    Each Playlist file MUST be identifiable either by the path component
    of its URI or by HTTP Content - Type.In the first case, the path MUST
    end with either.m3u8 or .m3u.In the second, the HTTP Content - Type
    MUST be "application/vnd.apple.mpegurl" or "audio/mpegurl".Clients
    SHOULD refuse to parse Playlists that are not so identified.
```
`class HLSClient` does this by checking HTTP requestline to end in ".m3u8" or ".m3u" and later in `HLSClient::run_cdn_response_parsing` method by checking Content-Type of the response. In paractice not everyone honors latest spec so several other Content-Type variations are checked. See `HLSClient::media_context_type_from_request_url` and `HLSClient::media_context_type_from_response_header`.

When status of response is 404 then response is not marked as MANIFEST type. It is normal to detect request as MANIFEST and response UNKNOWN.

If response is not MANIFEST then response is sent unmodified to VLC.

If response is MANIFEST then we need to modify URLs in content. When `Content-Length` header is defined by CDN and we modify URLs (we need to add a path segment) the `Content-Length` value is no longer correct. Because we decided to stream response to VLC keeping minimal latency we can't know correct `Content-Length` value when HTTP headers are sent to VLC. Because of this chunked encoding is used in sending response. Method to do this operation is `HLSClient::send_to_player_nossl_chunked`.

At the end of `HLSClient::run_cdn_response_parsing` method time measurement is taken. Time is measured from start of VLC's request to end of CDN response streamed to player.

`class CDNConnection` is used to make request to CDN and read response from socket. `class CDNConnectionSSL` is the same for SSL connection. mbedtls-2.16.2 library is used for SSL in `class CDNConnectionSSL`. Version used during dev is https://tls.mbed.org/download/mbedtls-2.16.2-apache.tgz. Just unpack it in root of the project, compiler and linker options should be already set in VC solution file.

HTTP redirects are handled by rewriting `Location` header so VLC is left to handle recursive redirect situations.

HTTP request and response parsing is done using `http-parser` library from node.js https://github.com/nodejs/http-parser . 


### Building:

Depends on https://tls.mbed.org/download/mbedtls-2.16.2-apache.tgz. Unpack in project dir .

Depends on ftp://sourceware.org/pub/pthreads-win32/prebuilt-dll-2-9-1-release/. Copy in project dir under pthreads-win32/ .

Depends on https://github.com/nodejs/http-parser.

Build using Visual Studio 2017 .

### Server usage:

```
HLSProxy.exe <host-to-listen-on> <port>
```

Exaple:
```
HLSProxy.exe 127.0.0.1 8080
```

Note that `0.0.0.0` is not valid parameter for `host` as this is later used to rewrite URLs.

If using VLC UI please note that you must enter `encoded-url` so stream goes through `HLSProxy` server.

### Running integration tests:

Scripts for integration test are located in `test_integration_py/` dir.

Compose list of test HLS sources and place them in a file. There is example file located `test_integration_py/hls_sources.txt`.

First start HLSProxy server. For example:
```
HLSProxy.exe 127.0.0.1 8080
```

Then start test suire with:
```
\test_integration_py>python test.py hls_sources.txt 127.0.0.1 8080
```

Each HLS source will be tested. libVLC will decode media for some time and report results.

```
Test run complete:
[TestOutcome PASSING http://dzeri.com/tmp/lsv2-1920x1080.mp4]
[TestOutcome PASSING https://bitdash-a.akamaihd.net/content/MI201109210084_1/m3u8s/f08e80da-bf1d-4e3d-8899-f0f6155f6efa.m3u8]
[TestOutcome PASSING http://dzeri.com/streamlabs/absolute.m3u8]
[TestOutcome PASSING http://dzeri.com/streamlabs/redirected_absolute.m3u8]
[TestOutcome INVALID_URL http://dzeri.com/s/o/m/e/w/h/e/r/e/notexisting_url.m3u8]
[TestOutcome INVALID_URL http://184.72.239.149/vod/smil:BigBuckBunny.smil/playlist.m3u8]
[TestOutcome INVALID_URL http://www.streambox.fr/playlists/test_001/stream.m3u8]
[TestOutcome FAILED https://mnmedias.api.telequebec.tv/m3u8/29880.m3u8]
```

### Status

```
Debuggable     #########
Maintainable   ##########
Understandable ########
Documented     #######
Portability    ########
```

### Limitations:

Other HTTP media streams may work (like MP4) but are not tested.

IPv6 not supported. Changes to socket logic is needed.

Commandline parsing is not robust.

TCP tuning not done.

Cookie handling is basic. Proper HTTP Cookie parser should be used in the future.


