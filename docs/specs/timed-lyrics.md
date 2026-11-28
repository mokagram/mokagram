# Local timed lyrics

## Assumptions

- Keep the existing lyrics source order. Prefer externally supplied timings to local recognition.
- Show the fraction of audio TinyASR has processed, not an estimate of time remaining. Show a separate status for audio preparation and lyric alignment.
- Keep a bounded local cache of successful timed transcripts. Reuse the transcript with the current lyrics source, so a provider change cannot restore old lyrics text.

## Acceptance criteria

- The full player and chat sidebar show plain lyrics while TinyASR adds timestamps. A status remains visible until processing finishes. If no source supplies lyrics, show a generation status instead.
- The progress bar uses completed audio duration. It does not advance while TinyASR prepares the audio or while the client aligns the lyrics.
- When multiple CPU threads are available, TinyASR can decode up to four existing song windows at once. It keeps the same language, words, and timestamps as sequential recognition. Voice message recognition is unchanged.
- Successful processing enables timed highlighting. Failed processing clears the status and leaves plain lyrics readable, or shows the existing missing lyrics state when there were none.
- Changing tracks clears the old status. Pending work for the old track does not replace the new lyrics.
- Replaying the same track after a client restart can reuse its timed transcript without reading or transcribing the audio again. An invalid or missing cache entry does not prevent recognition.
- The cache stores text and timing only. It does not store or upload audio, and it does not reuse transcripts across sign-ins to different accounts.
