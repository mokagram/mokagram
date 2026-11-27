# Tyanka Hikka track waffles

Only the Music Home detail card for SoundCloud track `2413072539` shows four waffle buttons, two on each side of the cover and title card. The other tracks and the artist page do not change.

- Each waffle has a transparent background and drifts independently. The artwork compresses on a pointer press and returns on release.
- Each button plays a distinct one-, two-, or three-bark pattern from the existing Tyanka Hikka voice pack. Activation does not change song playback or enable the chat sound setting.
- The buttons have accessible names, visible keyboard focus, and keyboard activation.

## Assumptions

- On narrow layouts, hide both sides rather than overlap the title or the cover.
- Follow the app's animation and power-saving settings. Use the notification volume for the short clips.
- Use the two unwatermarked attachments and two open-license photographs. Do not ship the watermarked reference.
- Reuse local audio when present. On the first click without local audio, use the existing SoundCloud download flow. Do not ship the song in the app.
