// What the page reads from the radio besides its state: the presets, the
// log, the DX catches, the sweeps and the stored settings. Each is read once
// for all the screens that show it, and kept, so a screen opened again shows
// it straight away and asks the radio nothing.
import { computed, effect, signal } from '@preact/signals';
import { useEffect } from 'preact/hooks';
import { getJson, getLines, getLog, getPresets, signedIn, state } from './radio.js';
import { act } from './act.js';

// A failed read is tried again with the first state that came at least this
// long after the state it was tried on: with polling, the next state; with
// the live state, about every tenth.
const RETRY_MS = 5000;

// One thing read from the radio. use() gives a screen { data, error, at }:
// data is null until it has come, and again after a read failed, so a screen
// says it is not known rather than show none; `at` is when that read was
// asked, as performance.now. peek() gives the same outside a screen, and
// `value` the same to a screen that cannot use a hook, drawn again when it
// changes. It is read only while a screen shows it, or `always`: when the
// first such screen opens, whenever key(state) moves, and at once by
// again(), after the page's own change. The key that change moves, in the
// next state, is then taken as read, since again() read it already. A key
// of undefined waits. A read refused for the PIN is tried again once the
// page signs in, not on the 5 s pace.
export function kept(key, read, always = false) {
  const got = signal({ data: null, error: null });
  const shown = signal(always ? 1 : 0); // how many screens show it now
  let seen = null; // the last state, and when it came
  let seenAt = 0;
  let readFor; // the key of the last read
  let triedAt = 0; // when the state of the last read came
  let busy = false;
  let last = 0; // the newest read: the answer to an older one is dropped
  let adopt = false; // the next state's key is taken as read

  async function readNow() {
    const mine = ++last;
    readFor = seen && key(seen);
    triedAt = seenAt;
    busy = true;
    const at = performance.now();
    let answer;
    try {
      answer = { data: await read(), error: null, at };
    } catch (error) {
      answer = { data: null, error, at };
    }
    if (mine !== last) return;
    busy = false;
    got.value = answer;
  }

  effect(() => {
    const s = state.value;
    const fresh = s !== seen;
    if (fresh) {
      seen = s;
      seenAt = performance.now();
    }
    const k = s && shown.value ? key(s) : undefined;
    if (k === undefined) return;
    // Only the first state after again() is taken as read. When its key did
    // not move, as after a change the radio refused, the next move is read.
    if (fresh && adopt) {
      adopt = false;
      if (k !== readFor) {
        readFor = k;
        return;
      }
    }
    const { error } = got.peek();
    const due = error && !busy && (error.status === 403 ? signedIn.value : seenAt - triedAt >= RETRY_MS);
    if (k !== readFor || due) readNow();
  });

  function use() {
    useEffect(() => {
      shown.value++;
      return () => shown.value--;
    }, []);
    return got.value;
  }
  return {
    use,
    again() {
      adopt = true;
      return readNow();
    },
    peek: () => got.peek(),
    get value() {
      return got.value;
    },
    set: (data) => (got.value = { data, error: null }),
  };
}

// The presets, read again when the radio's list changes: its count, or its
// `rev`, which a rename or a move on the radio changes too.
export const presets = kept((s) => `${s.pst?.n}:${s.pst?.rev}`, getPresets);

// The whole station log, and its newest three for Home, newest first, read
// again when the log changes: its count, or its `rev`, which also moves when
// a full log takes a new entry. The whole log is not read while a DX scan
// runs, which can log a station every few seconds; it is read once the scan
// ends.
const logKey = (s) => `${s.log?.n}:${s.log?.rev}`;
const scanning = (s) => s.dx?.scn && s.dx.scn !== 'idle';
export const stationLog = kept((s) => (scanning(s) ? undefined : logKey(s)), getLog);
export const lastHeard = kept(logKey, async () => (await getLog(3)).reverse());

// False when the radio's log storage did not start. GET /api/log reads as
// an empty log then; only the state tells the two apart.
export const logFits = computed(() => state.value?.log?.fit !== false);

// The stations DX mode caught, read again when its `rev` moves.
export const catches = kept((s) => s.dx?.rev, () => getLines('/api/dx').then(([, ...list]) => list));

// DX mode's last sweep, read again when `swr` moves, not while it sweeps.
export const dxSweep = kept((s) => (s.dx?.swp ? undefined : s.dx?.swr), () => getJson('/api/dx/sweep'));

// The band scope's last sweep, for Home and Band scope, read again when its
// `rev` moves, not while it sweeps.
export const scopeSweep = kept((s) => (s.scp?.run ? undefined : s.scp?.rev), () => getJson('/api/scope'));

// The radio's stored settings, GET /api/settings, which needs the PIN. Read
// again whenever the radio stores its settings (`asv.n` counts them), so a
// change on the radio's own menu shows too, and when its settings start to
// differ from the stored ones (`asv.dif`), for a change on the menu while
// saving by itself is off. `needPin` is true while the radio refuses it for
// want of the PIN.
const settings = kept((s) => `${s.asv?.n}:${s.asv?.dif}`, () => getJson('/api/settings'));

// The changes this page made, each drawn over the settings read until a read
// asked after the radio took it, which holds it already.
const drawn = signal([]); // { fields, takenAt }
const unread = (at) => drawn.value.filter((d) => !(d.takenAt < at));

export function useStoredSettings() {
  const { data, error, at } = settings.use();
  const stored = data && Object.assign({}, data, ...unread(at).map((d) => d.fields));
  return { stored, set, error, needPin: error?.status === 403 };
}

// A change to the stored settings: `fields` sent to `path`, and `draw` drawn
// over the settings at once; `expect`, if given, is drawn over the state too.
// A refusal takes back only this drawing, so a read that came in between
// stays. Resolves true when the radio took it.
async function set(fields, path = '/api/settings', draw = fields, expect) {
  const mark = { fields: draw, takenAt: Infinity };
  drawn.value = [...unread(settings.peek().at), mark];
  const took = await act(path, fields, expect);
  if (took) mark.takenAt = performance.now();
  else drawn.value = drawn.value.filter((d) => d !== mark);
  return took;
}
