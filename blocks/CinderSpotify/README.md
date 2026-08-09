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
| **Playback** | **not started** |
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

## Playback, when it comes

Playback is the part that cannot be done with the Web API. On iOS it means the
**App Remote SDK**, which controls the user's installed Spotify app rather than
playing audio itself, and requires Spotify Premium. That is a binary framework
dependency the Web API work deliberately avoided, which is why sign-in and the
library are done first.

Mapping the remaining `Player` surface onto App Remote:

| CinderIPod | App Remote |
| --- | --- |
| `play(playlist)` / `play(playlist, index)` | `playUri:` with the context URI, then skip to index |
| `pause()`, `skipNext()`, `skipPrev()` | direct equivalents |
| `setPlayheadTime()` / `getPlayheadTime()` | `seekToPosition:` / player state |
| `setShuffleMode()` / `setRepeatMode()` | `setShuffle:` / `setRepeat:` |
| `getPlayingTrack()` | player state's current track |
| `registerTrackChanged` etc. | player state subscription |

The good news for the visuals: Planetary never touches audio samples. There is
no FFT anywhere in it — the only playback signal the rendering uses is
`getPlayheadTime()`, which App Remote provides.
