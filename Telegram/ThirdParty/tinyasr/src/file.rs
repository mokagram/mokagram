//! A finished recording: only the full-context pass, piece by piece.
//!
//! Silero VAD marks speech over the whole file first. Then each piece runs from the end of the last one to the end of the sentence: a pause where
//! the end-of-turn head says the speaker has finished (asked at 0.24 s into each pause, as live), or 3 s of silence; at
//! most 20 s. It is decoded with the live session's pause compression and left context; a piece cut at 20 s keeps
//! its words up to the last . ? ! (else up to its longest pause) and the rest is decoded again with the next piece,
//! as the live session does.
//!
//! Music uses overlapping windows instead. Speech VAD is used only to choose a language, not to discard audio.

use alloc::format;
use alloc::vec::Vec;

use crate::model::Token;

use crate::session::{CAP, FR, SR, TAIL, compress, longest_pause};
use crate::text::{W, balance_quotes, cap, end_mark, is_end, show_quotes, spans, trim_trail, units, words};
use crate::vad::{CHUNK, VadState};
use crate::{Models, Word, allowed};

/// Frames of silence that end a sentence.
const LONG: usize = 75;
const MUSIC_WINDOW: usize = (10 * SR / FR) as usize;
const MUSIC_CONTEXT: usize = (SR / FR) as usize;
const LANGUAGE_PROBE: usize = (5 * SR / FR) as usize;

/// Recognition of a whole recording; an iterator over its pieces' final words, in order.
pub struct Transcription<'m> {
    m: &'m Models,
    audio: Vec<f32>, // whole frames
    frames: usize,
    music: bool,
    vf: Vec<bool>,
    lang: usize,
    langs: Vec<usize>, // file speech can choose a different language in each piece
    beam: usize,
    open0: usize, // frame where the next piece starts
    ctx: usize,   // frames of left context for it
    ended: bool,
    last_end: f64, // end of the last word given out (s)
}

impl<'m> Transcription<'m> {
    /// `audio`: 16 kHz. `langs`: as for [`crate::Session::new`] (with several, the recognizer picks for each piece).
    /// `beam`: beam width of the decoding ([`crate::DEFAULT_BEAM`]; 1: greedy, fastest).
    pub fn new(m: &'m Models, audio: &[f32], langs: &[&str], beam: usize) -> Transcription<'m> {
        Self::new_file(m, audio, langs, beam, false)
    }

    /// Singing: decode the entire recording in overlapping windows using one language.
    pub fn new_music(m: &'m Models, audio: &[f32], langs: &[&str], beam: usize) -> Transcription<'m> {
        Self::new_file(m, audio, langs, beam, true)
    }

    fn new_file(m: &'m Models, audio: &[f32], langs: &[&str], beam: usize, music: bool) -> Self {
        let frames = audio.len().div_ceil(FR as usize);
        let samples = frames * FR as usize;
        let size = samples + if music { TAIL } else { 0 };
        let mut a = Vec::with_capacity(size);
        a.extend_from_slice(audio);
        a.resize(size, 0.0);
        let langs = allowed(m, langs);
        let vf = if !music || langs.len() > 1 { speech_flags(m, &a[..samples]) } else { Vec::new() };
        let lang = if music && langs.len() > 1 {
            m.with_threads(|| music_language(m, &a, &vf, &langs))
        } else {
            langs[0]
        };
        let vf = if music { Vec::new() } else { vf };
        Self { m, audio: a, frames, music, vf, lang, langs, beam: beam.max(1), open0: 0, ctx: 0, ended: true, last_end: 0.0 }
    }

    /// The language's code (with several: of the last piece).
    pub fn lang(&self) -> &str {
        self.m.code(self.lang)
    }

    /// Audio length (s).
    pub fn duration(&self) -> f32 {
        (self.frames as i64 * FR) as f32 / SR as f32
    }

    /// Where the next piece starts (s): progress.
    pub fn position(&self) -> f32 {
        (self.open0 as i64 * FR) as f32 / SR as f32
    }

    /// The words out, timed by [`spans`] from the last word given out.
    fn out(&mut self, ws: Vec<W>) -> Vec<Word> {
        let vf = &self.vf;
        let ws = if self.m.asr.langs[self.lang].syllables { units(ws) } else { ws }; // Khmer: timed syllables
        let sp = spans(&ws, self.last_end, |f| usize::try_from(f).ok().and_then(|i| vf.get(i).copied()));
        if let Some(&(_, e)) = sp.last() {
            self.last_end = e as f64;
        }
        ws.into_iter().zip(sp).map(|(w, (start, end))| Word { text: show_quotes(w.w, &self.m.asr.langs[self.lang]), start, end, join: w.join }).collect()
    }
}

impl Iterator for Transcription<'_> {
    type Item = Vec<Word>;

    fn next(&mut self) -> Option<Vec<Word>> {
        let m = self.m;
        m.with_threads(|| if self.music { self.next_music() } else { self.next_here() })
    }
}

impl Transcription<'_> {
    fn next_music(&mut self) -> Option<Vec<Word>> {
        let begin = self.open0;
        if begin >= self.frames {
            return None;
        }
        let end = (begin + MUSIC_WINDOW).min(self.frames);
        self.open0 = end;
        let from = begin.saturating_sub(MUSIC_CONTEXT);
        let to = ((end + MUSIC_CONTEXT) * FR as usize).min(self.audio.len());
        let tokens = self.m.asr.full(&self.audio[from * FR as usize..to], self.lang, self.beam);
        let seconds = |frame| (frame as f64) * FR as f64 / SR as f64;
        let mut ws = words(&self.m.asr.pieces, &tokens, seconds(from));
        // Adjacent windows hear the seam from both sides but only own their central interval.
        let first = seconds(begin) - 0.02;
        let last = seconds(end) - 0.02;
        ws.retain(|w| w.t >= first && (end == self.frames || w.t < last));
        Some(self.out(ws))
    }

    fn next_here(&mut self) -> Option<Vec<Word>> {
        let n = self.vf.len();
        loop {
            let first = (self.open0..n).find(|&i| self.vf[i])?;
            // the piece: up to 3 s of silence, at most CAP of audio from its first speech
            let cap_end = first + (CAP / FR) as usize;
            let (mut b, mut quiet, mut eou) = (first, 0, false);
            while b < n && b < cap_end && quiet < LONG {
                quiet = if self.vf[b] { 0 } else { quiet + 1 };
                b += 1;
                // 0.24 s into a pause: has the speaker finished? (the turn so far, <= 30 s; ended at 0.48 s of silence)
                if quiet == 6 && self.m.asr.has_head() && self.vf[b..(b + 6).min(n)].iter().all(|v| !v) {
                    let from = self.open0.max(b.saturating_sub(750));
                    if self.m.eou(&self.audio[from * FR as usize..b * FR as usize], self.lang).is_some_and(|p| p > 0.5) {
                        b = (b + 6).min(n);
                        eou = true;
                        break;
                    }
                }
            }
            let end = eou || quiet >= LONG; // the sentence is over: a dot if the model wrote none
            let full = !end && b >= cap_end && b < n; // cut short: more speech may follow
            let a = self.open0.saturating_sub(self.ctx);
            let (x, map) = compress(&self.audio[a * FR as usize..b * FR as usize], &self.vf[a..b], a as i64);
            let (tk, picked) = self.m.full(&x, &self.langs, self.beam);
            if let Some(l) = picked {
                self.lang = l;
            }
            let tk: Vec<Token> = tk.iter().map(|t| Token { frame: map[(t.frame as usize).min(map.len() - 1)] as u32, ..*t }).collect();
            // the left context's own words are dropped
            let o0 = (self.open0 as i64 * FR) as f64 / SR as f64 - 0.02;
            let mut ws: Vec<W> = words(&self.m.asr.pieces, &tk, 0.0).into_iter().filter(|w| w.t >= o0).collect();
            let (keep, next, ctx) = if !full || ws.len() < 2 {
                // a sentence end or the end of the audio
                if end {
                    if let Some(last) = ws.last_mut() {
                        if !is_end(&last.w) {
                            last.w = format!("{}{}", trim_trail(&last.w), end_mark(&self.m.asr.langs[self.lang]));
                        }
                    }
                }
                (ws.len(), b, 0)
            } else if let Some(j) = (0..ws.len() - 1).rev().find(|&j| is_end(&ws[j].w)) {
                (j + 1, cut(&ws, j), (SR / FR) as usize)
            } else {
                let j = longest_pause(&ws).unwrap_or(ws.len() - 2);
                (j + 1, cut(&ws, j), (4 * SR / FR) as usize)
            };
            let next = next.max(self.open0 + 1);
            ws.truncate(keep);
            self.open0 = next;
            self.ctx = ctx;
            if ws.is_empty() {
                continue;
            }
            let ended = is_end(&ws[ws.len() - 1].w);
            if self.ended {
                ws[0].w = cap(&ws[0].w);
            }
            let out = self.out(balance_quotes(ws));
            self.ended = ended;
            return Some(out);
        }
    }
}

fn speech_flags(m: &Models, audio: &[f32]) -> Vec<bool> {
    let mut flags = Vec::with_capacity(audio.len() / FR as usize);
    let (mut st, mut on, mut done) = (VadState::new(), false, 0);
    for frame in 0..audio.len() / FR as usize {
        let end = (frame + 1) * FR as usize;
        while done + CHUNK <= end {
            let p = m.vad.chunk(&mut st, &audio[done..done + CHUNK]);
            on = p > 0.5 || (on && p > 0.35);
            done += CHUNK;
        }
        flags.push(on);
    }
    flags
}

fn music_language(m: &Models, audio: &[f32], voiced: &[bool], langs: &[usize]) -> usize {
    let mut votes = vec![0; langs.len()];
    let mut best = (0, langs[0]);
    let third_size = voiced.len().div_ceil(3).max(1);
    for (i, third) in voiced.chunks(third_size).enumerate() {
        let Some((j, window)) = third.chunks(LANGUAGE_PROBE).enumerate()
            .max_by_key(|(j, block)| (block.iter().filter(|&&flag| flag).count(), block.len(), core::cmp::Reverse(*j))) else { continue };
        let first = (i * third_size + j * LANGUAGE_PROBE) * FR as usize;
        let end = first + window.len() * FR as usize;
        if let (_, Some(lang)) = m.full(&audio[first..end], langs, 1) {
            let index = langs.iter().position(|&l| l == lang).unwrap();
            votes[index] += 1;
            if votes[index] > best.0 {
                best = (votes[index], lang);
            }
        }
    }
    best.1
}

/// Frame halfway between word j and j + 1.
fn cut(ws: &[W], j: usize) -> usize {
    ((ws[j].te + ws[j + 1].t) / 2.0 / 0.04 + 0.5) as usize
}
