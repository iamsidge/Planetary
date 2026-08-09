# CinderSpotify

A Spotify backend for Planetary, presenting the same shapes `CinderIPod` does
so the app's model and rendering are untouched.

## Why this shape

Planetary never references `MPMediaItem` — not once in ~12k lines of `src/`.
Everything goes through `ci::ipod::Track`, `Playlist` and `Player`. So Spotify
support is a new block implementing an existing interface rather than surgery
through the app, and this block deliberately mirrors that interface method for
method.

The surface the app actually uses is small: four free functions
(`getArtists`, `getPlaylists`, `getAlbumsWithArtistId`,
`getAlbumPlaylistWithArtistId`), thirteen accessors, and eighteen `Player`
methods.

## Status

| | |
| --- | --- |
| Sign-in (OAuth PKCE) | done |
| Token refresh | done |
| Followed artists | done |
| User playlists | done |
| Albums and tracks per artist | done |
| Artwork | done, asynchronous — see below |
| Playback (transport, state, modes) | done |
| Wired into the app | no — still builds against CinderIPod |

It compiles and is built as part of the app, but nothing calls it yet. The
swap happens once playback exists, so the app is never left half-migrated.

## Configuration

No credentials are committed. Add to the app's `Info.plist`:

```xml
<key>SpotifyClientID</key>
<string>your-client-id</string>
<key>SpotifyRedirectURI</key>
<string>planetary://spotify-callback</string>
```

The redirect scheme must also be registered under `CFBundleURLTypes`, or the
callback never returns to the app. Create the client id at
<https://developer.spotify.com/dashboard>.

## Design notes

**String ids → uint64_t.** Planetary identifies artists, albums and tracks by
`uint64_t`, inherited from `MPMediaItem`'s persistent ids, and those ids are
load-bearing: node lookup, equality and selection all use them. Spotify uses
base-62 strings. `SpotifyId.h` assigns each string a stable `uint64_t` and
remembers the mapping both ways, so playback can turn a node's id back into a
URI. Collisions are probed rather than ignored — two artists silently sharing
an id would merge their nodes. Ids are stable within a run but not across
runs; nothing persists them today, but anything that starts caching them must
not assume otherwise.

**PKCE, not a client secret.** A shipped app cannot keep a secret. PKCE
replaces it with a random verifier held in memory, sending only its SHA-256
hash. Consent is presented via `ASWebAuthenticationSession`, so the flow runs
outside our process: the user's existing Spotify cookies apply, and the app
never sees their credentials.

**The library loaders block.** `getArtists` and friends make synchronous
network calls, because their callers are Planetary's background `TaskQueue`
tasks and are written to return results synchronously — the same contract the
iPod versions had. **Never call them from the main thread.**

**Artwork is the one interface mismatch that could not be papered over.**
`ci::ipod::Track::getArtwork` is synchronous and returns a `Surface`, which a
local library can honour instantly. Spotify returns a URL. Blocking a node-
building thread on a download would stall it, so `getArtwork` returns the
cached image if present and otherwise returns an empty `Surface` and starts a
fetch. Callers must treat empty as "not yet" and ask again. Planetary already
tolerates this, because it falls back to `mNoAlbumArtSurface`.

**Two values are fabricated, and say so.** `getPlayCount()` returns 0 —
Spotify exposes no per-user play count, and there is nothing to approximate it
with. `getStarRating()` is derived from Spotify's 0–100 popularity because
Planetary drives track glow from the rating and a constant would flatten the
visuals; it is not the user's own rating and must not be presented as one.

## Playback

`SpotifyPlayer.h` mirrors `ci::ipod::Player`, so KeplerApp's ~18 call sites are
unchanged.

**Web API, not the App Remote SDK.** The obvious route was Spotify's App Remote
framework. Comparing them: App Remote drives the user's installed Spotify app,
the Web API drives whichever Spotify device is active. *Neither plays audio
itself* — Planetary does not become an audio player either way. Since the
capability is identical, App Remote's binary `.xcframework` buys nothing while
costing a vendored blob and the ability to run in the simulator. So this is
plain HTTPS on the auth we already have.

**What it needs to work:** Spotify Premium, and an active Spotify device —
their phone, desktop app, or a speaker. When nothing is active this transfers
playback to the first available device; with no devices at all, `play()` fails
and `getPlayStateString()` says why, which is what Planetary already displays.

**Polling, because the Web API has no push channel.** State is fetched once a
second. That is also how playback started on another device is noticed.
`getPlayheadTime()` interpolates between polls — Planetary drives visuals from
the playhead and would visibly step at 1Hz otherwise — and a seek updates the
cached position immediately rather than letting the visuals jump backwards for
up to a second.

Callbacks fire on the main thread, since Planetary's handlers touch scene state.

Three places where Spotify and the iPod interface do not line up:

- **Shuffle** is a boolean in Spotify. `ShuffleModeSongs` and
  `ShuffleModeAlbums` both map to on.
- **Stop** does not exist; `stop()` pauses, which is what Planetary means by it.
- **`registerLibraryChanged`** never fires. Spotify has no library-change
  notification. It is kept so the registration compiles.

One piece of luck worth recording: Planetary never touches audio samples. There
is no FFT anywhere in it, so the only playback signal the rendering needs is
`getPlayheadTime()`.
