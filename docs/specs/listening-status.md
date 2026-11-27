# Live listening status

## Assumptions

- Viewing and sharing have separate settings. Both start off. Viewing sends user IDs from the open profile, direct-chat header, visible chat rows, and visible message avatars to the bot. The settings apply to this installation and explain what the bot receives.
- Share a track title and artist from the built-in MokaGram player. Include the album title only when playback comes from a release marked as an album. Include the public SoundCloud track ID when available. Include Telegram tracks only when forwarding is allowed. Omit audio, links, chat IDs, and playback position.
- Send listening-status requests to `@mokagent` using Telegram inline queries. Use the existing bot destination for all other requests.
- Use a static accent ring on avatars in profiles, direct-chat headers, chat lists, and visible messages. Keep story rings and saved profile music available.

## Acceptance criteria

- The profile shows a track card above Channel with the track title and artist. The chat header shows the usual status and a ring around the avatar.
- The track card takes its tint from the profile background, including collectible backdrops. It uses the theme surface when the profile has no custom background.
- The profile avatar shows a static listening ring.
- The regular chat list shows the ring on direct-chat avatars. Open chats and threads show the same ring on visible message avatars from eligible users. The ring follows the displayed sender and leaves avatar clicks intact.
- A confirmed listening status appears in any visible view of that user in the same signed-in account. The status keeps its original expiry time across views.
- The profile card uses the album cover in My Profile when available. Other profiles use the current track cover when available.
- Starting an album from its Play button queues all playable tracks in release order. Selecting a track on the album page queues the same album and starts at the selected track, even if the previous queue was edited.
- During album playback, My Profile and other viewers show the current track title on the first line and `Album · <album title>` on the second line. The first line changes as the album advances. Playing the same track on its own shows the track title and artist without the album label.
- Activating the card or a ringed avatar opens the track in the player when its public SoundCloud ID is available. Other tracks open music search with their shared text.
- The track and ring disappear after a pause, stop, or opt-out reaches the viewer. The status also expires when the publisher loses its connection or closes the client.
- Switching chats never shows a previous user's track. Bot errors and invalid responses leave the normal profile and chat status usable.
- A reply that does not confirm a track or its absence leaves the last confirmed status in place until its lease expires. An unacknowledged publish can refresh sooner than the normal publish interval.

## Bot contract

- `presence:v1:put:<writer>:<base64url>` sets a 35-second lease for the authenticated inline query sender. The writer is a random hexadecimal value of at most 16 digits. The payload encodes a UTF-8 track label of up to 150 bytes, without padding. The label starts with the title and includes the artist when available. An empty label is invalid.
- Album playback uses the same v1 label field. The client joins the track title, artist (which may be empty), and album title with `U+2063` followed by ` · `. Field text excludes `U+2063`. Keep both titles when shortening the label to fit. The bot stores and returns the label without interpreting its fields. Older labels still show the track title and artist.
- `presence:v1:clear:<writer>` removes the authenticated sender's lease when its writer matches. A clear from an earlier client must leave a newer client's lease in place.
- `presence:v1:get:<user_id>` returns a lease for a positive Telegram user ID. Return no result when the lease is absent or expired.
- `presence:v1:put:<writer>:<soundcloud_id>:<base64url>` adds a validated public SoundCloud ID to the same lease.
- `presence:v1:list:<user_id>,...` gets up to 12 visible peers in one request. A successful reply has one article per requested user with ID `presence-v1:<user_id>`. Its description contains the remaining milliseconds, followed by a colon and the public track ID when present. Use description `0` to confirm that the user has no active lease.
- Return one article with ID `presence-v1`. For a read, put the label in `title` and the remaining lease in milliseconds in `description`. A successful write or clear only needs the ID.
- Set `cache_time` to `0` and `is_personal` to `true`. Rate limit by sender. Derive the publisher identity from the inline query sender. Keep only the current lease and remove expired records. Never log the track label.
