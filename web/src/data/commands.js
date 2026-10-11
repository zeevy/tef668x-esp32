// The radio's everyday commands, each with its route and, when its result is
// known, what it draws at once. Every place that tunes, seeks, steps, mutes,
// sets the volume, recalls, stores or logs sends it through here, so they all
// do it the same way. Each resolves true when the radio took it.
import { effect, signal } from '@preact/signals';
import { change, state, volHeld } from './radio.js';
import { bandAt, bandOf } from './bands.js';
import { presets } from './reads.js';
import { freq, level, printable, unit } from './format.js';
import { PRESET_NAME_MAX } from './limits.js';
import { act, refused, say } from './act.js';

// What a tune does to the state, as the radio shows it straight after: the
// band, its own step and width when the band changes, the preset `pst` (0
// for none), a preset's own width `bw` when it has one, and no RDS yet.
function tuned(n, khz, band, pst, bw) {
  const t = n.tun;
  const b = band !== t.bnd && bandOf(band);
  if (b) Object.assign(t, { stp: b.step, bws: b.width });
  Object.assign(t, { bnd: band, khz, f: freq(khz, band), unt: unit(band), pst, rds: {} });
  if (bw) t.bws = bw;
}

// A tune to a frequency in kHz, on the band the radio puts it on. The radio
// shows the preset that holds it, the lowest slot when more than one does;
// the presets as last read say which. Not drawn while the bands are not
// known: the state is read back at once instead.
export function tune(khz) {
  const band = bandAt(khz);
  const pst = presets.peek().data?.find((p) => p.band === band && p.khz === khz)?.slot ?? 0;
  return act('/api/tune', { khz }, band && ((n) => tuned(n, khz, band, pst)));
}

// The frequency the dial rested on, 3 s or more, before it last moved, from
// any source: the page, the knob or a PC. Tune back goes there, and from
// there back again. A seek, a scan or a turn of the knob passes channels too
// fast to count as one.
const REST_MS = 3000;
export const before = signal(null);
let dial = null;
let since = 0;
effect(() => {
  const khz = state.value?.tun?.khz;
  if (khz == null || khz === dial) return;
  if (dial != null && performance.now() - since >= REST_MS) before.value = dial;
  dial = khz;
  since = performance.now();
});
export function tuneBack() {
  const to = before.value;
  if (to == null) return false;
  before.value = dial;
  return tune(to);
}

// The station as one line, for a log or a message:
// 104.30 MHz PI 9201 PS RADIO 1 54.2 dBµV.
export function stationLine(t) {
  const r = t.rds ?? {};
  return [`${t.f} ${t.unt}`, r.pi && `PI ${r.pi}`, r.ps?.trim() && `PS ${r.ps.trim()}`, level(t.sig)].filter(Boolean).join(' ');
}

// A text put on the clipboard. Browsers give their clipboard call only to
// pages served over https, and the radio serves http, so this uses the old
// copy command, which still works over http.
export function copy(text) {
  const was = document.activeElement;
  const box = document.createElement('textarea');
  box.value = text;
  box.setAttribute('readonly', '');
  box.style.position = 'fixed';
  box.style.opacity = '0';
  document.body.append(box);
  box.select();
  const done = document.execCommand('copy');
  box.remove();
  was?.focus();
  say(done ? `Copied: ${text}` : 'This browser did not let the page copy.', done ? 'good' : 'bad');
}

export const recall = (p) => act('/api/presets', { do: 'recall', slot: p.slot }, (n) => tuned(n, p.khz, p.band, p.slot, p.bw));

// Where a step or a seek lands depends on the tuning mode and the band, so
// neither is drawn: the state is read back at once.
export const step = (n) => act('/api/step', { stp: n });
export const seek = (up) => act('/api/seek', { dir: up ? 'up' : 'down' });

export const mute = (on) => act('/api/mute', { on: +on }, (n) => (n.tun.mut = on));

// The volume being moved, `volHeld`, is let go once the radio has this one,
// unless a newer one is being moved by then.
export async function volume(db) {
  const took = await act('/api/volume', { db }, (n) => (n.tun.vol = db));
  if (volHeld.peek() === db) volHeld.value = null;
  return took;
}

// The station on the dial as a new preset, with its name, on the band the
// dial is on: 68 MHz on FM stays FM, though OIRT holds it too. The radio
// picks the slot, the lowest free one, from the list it holds, and refuses a
// station that is a preset already, so a list this page read before cannot
// make it write over one. Says the radio's own words either way.
export async function storeStation(t) {
  const name = printable(t.rds?.ps?.trim() ?? '', PRESET_NAME_MAX);
  try {
    say(await change('/api/presets', { do: 'add', khz: t.khz, bnd: t.bnd, bw: t.bws, name }));
    presets.again();
    return true;
  } catch (e) {
    refused(e);
    return false;
  }
}

// Logs the station on the dial, and says which once the radio has. Read when
// asked, not while drawing, so a screen that logs need not draw again with
// every state.
export async function logStation() {
  const t = state.peek()?.tun;
  const took = await act('/api/log', {});
  if (took) say(`Logged ${t?.f} ${t?.unt}.`);
  return took;
}
