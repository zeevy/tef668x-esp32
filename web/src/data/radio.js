// Every call to the radio goes through this file, so the way the page gets
// its data can change in one place.
import { signal } from '@preact/signals';
import { csvFields } from './csv.js';
import { withMotion } from './motion.js';

// The last /api/state, or null before the first answer.
export const state = signal(null);
// 'ok', 'slow' (an answer is late), or 'lost' (the radio stopped answering).
export const link = signal('ok');
// Signed in with the PIN: a write the radio refused for the PIN sets it false.
export const signedIn = signal(null);
// True while the PIN prompt should be open: the owner asked for it, or the
// radio refused a change for want of the PIN.
export const pinAsked = signal(false);
// The volume under a finger or the arrow keys, drawn by every volume control
// until it is sent; null otherwise, so the radio's own reading shows.
export const volHeld = signal(null);

// The radio sends its state over a WebSocket twice a second. Asking for it
// is the fallback, while the socket is not open: each read holds the radio's
// own screen for 85 to 280 ms, so at one a second the screen stutters, and
// every 5 seconds is the pace.
const POLL_MS = 5000;
// A new socket 5 s after one closed; after one that never opened, such as
// the third page the radio turns away, twice as long each time, up to 60 s.
const LIVE_RETRY_MS = 5000;
const LIVE_RETRY_MAX_MS = 60000;
// The radio sends nothing while a level sweep reads the band, 7.5 s at most
// on MW, so a socket quiet for longer has lost the radio: it restarted or
// left the Wi-Fi without closing the connection.
const SILENCE_MS = 10000;
// A state had this soon after the radio took a change may still be from
// before it: the radio task takes a command within its 100 ms round.
const SETTLE_MS = 300;
const TIMEOUT_MS = 10000;

// Asks the radio and reads its whole answer, within TIMEOUT_MS. When it
// does not answer, says so in plain words: the browser's own, such as
// "Failed to fetch", mean nothing to a person.
async function call(path, init = {}) {
  const ctrl = new AbortController();
  const timer = setTimeout(() => ctrl.abort(), TIMEOUT_MS);
  try {
    const res = await fetch(path, { credentials: 'same-origin', ...init, signal: ctrl.signal });
    return { res, text: await res.text() };
  } catch {
    throw new Error('The radio did not answer.');
  } finally {
    clearTimeout(timer);
  }
}

// A refusal as an error: the radio's own words, or its status when it gave
// none.
const refusal = (res, words) => Object.assign(new Error(words || `The radio refused it (${res.status}).`), { status: res.status });

// GET a route as text; throws on an answer that is not 2xx.
async function getText(path) {
  const { res, text } = await call(path);
  // A refused read means the session is gone, as a refused write does.
  if (res.status === 403) signedIn.value = false;
  if (!res.ok) throw refusal(res, text.trim());
  return text;
}

// GET a route and parse it as JSON, as getText.
export const getJson = async (path) => JSON.parse(await getText(path));

// GET a route of one JSON object per line, such as /api/dx.
export async function getLines(path) {
  return (await getText(path)).split('\n').filter(Boolean).map((l) => JSON.parse(l));
}

// The presets, from the CSV the radio also offers as a download.
export async function getPresets() {
  const [, ...rows] = (await getText('/api/presets.csv')).split(/\r?\n/).filter(Boolean);
  return rows.map((row) => {
    const [slot, band, khz, bw, name, pi] = csvFields(row);
    return { slot: +slot, band, khz: +khz, bw: +bw, name, pi };
  });
}

// The station log, oldest first, or only its newest `last`. The first line
// is only the count. A count over 0 with no entries after it is a read cut
// short, not an empty log.
export async function getLog(last) {
  const [head, ...entries] = await getLines(last ? `/api/log?last=${last}` : '/api/log');
  if (head?.n > 0 && !entries.length) throw new Error('The log came back with no entries.');
  return entries;
}

// POST form fields, as every write route takes them. Resolves to the radio's
// plain text answer; throws with that answer, and the status, when it refuses.
export function post(path, fields = {}) {
  return postBody(path, new URLSearchParams(fields), 'application/x-www-form-urlencoded');
}

// The plain words of an answer: some routes answer with a short HTML page
// made for the browser, whose words are the radio's reason.
function wordsOf(text, type = '') {
  if (!type.includes('html')) return text.trim();
  // A space before each tag, so a heading and the line after it do not run
  // together.
  return new DOMParser().parseFromString(text.replace(/</g, ' <'), 'text/html').body.textContent.replace(/\s+/g, ' ').trim().slice(0, 200);
}

// A write refused for want of the PIN: the page is signed out, and the PIN
// prompt opens.
function denied(status) {
  if (status !== 403) return;
  signedIn.value = false;
  pinAsked.value = true;
}

// POST a body as it is, such as a CSV file, answered as post is.
export async function postBody(path, body, type) {
  const { res, text } = await call(path, { method: 'POST', headers: { 'Content-Type': type }, body });
  const words = wordsOf(text, res.headers.get('content-type') ?? '');
  denied(res.status);
  if (!res.ok) throw refusal(res, words);
  return words;
}

// POST a form with a file in it, such as a firmware, answered as postBody
// is. onProgress hears how much has gone, 0 to 1, while it is sent. No time
// limit: a firmware takes longer than any other write to reach the radio.
export function upload(path, form, onProgress) {
  return new Promise((resolve, reject) => {
    const x = new XMLHttpRequest();
    x.open('POST', path);
    x.upload.onprogress = (e) => e.lengthComputable && onProgress?.(e.loaded / e.total);
    x.onload = () => {
      const text = wordsOf(x.responseText, x.getResponseHeader('content-type') ?? '');
      denied(x.status);
      if (x.status < 200 || x.status >= 300) reject(refusal(x, text));
      else resolve(text);
    };
    x.onerror = () => reject(new Error('The file did not reach the radio.'));
    x.send(form);
  });
}

// Asks a route that needs the PIN, to learn whether this browser is signed in.
async function checkSignIn() {
  try {
    const { res } = await call('/api/settings');
    signedIn.value = res.ok;
  } catch {
    // Not known while the radio does not answer.
  }
}

// Signs in with the PIN. Resolves on success; throws with a short reason.
export async function signIn(pin) {
  const { res } = await call('/auth', { method: 'POST', body: new URLSearchParams({ pin }) });
  if (res.status === 429) {
    throw new Error(`Too many tries. Wait ${res.headers.get('Retry-After') || 'a few'} seconds.`);
  }
  if (!res.ok) throw new Error(res.status === 400 ? 'A PIN is six digits.' : 'That PIN is not right.');
  signedIn.value = true;
}

let running = false; // the page is in view
let open = false; // a poll is waiting for its answer
let again = false; // poll again as soon as the open one ends
let timer = null;
let socket = null; // the live state's socket, while one is open or opening
let retry = null;
let retryMs = LIVE_RETRY_MS;
let silence = null;
// The last state the radio gave, and the changes drawn over it until the
// radio has surely taken them, so a state from before one cannot undo it on
// the screen, and a refusal can take its change back off.
let fromRadio = null;
let drawn = []; // { patch, takenAt }
// When the state shown was had. A read that started before a state the
// socket brought answers with an older one. performance.now, which a clock
// set back does not move.
let newest = -Infinity;

const now = () => performance.now();
const live = () => socket?.readyState === WebSocket.OPEN;

// A state with the page's own changes drawn over the radio's is marked
// `patched`: its readings are the radio's last, not new ones, so the DX
// meter does not count them.
function show() {
  const s = drawn.length ? structuredClone(fromRadio) : fromRadio;
  for (const d of drawn) d.patch(s);
  if (drawn.length) s.patched = true;
  state.value = s;
}

// What a change worth seeing happen is: another frequency, band, preset or
// mute. The readings that change in every state are not.
const station = (s) => s && `${s.tun?.bnd} ${s.tun?.khz} ${s.tun?.pst} ${s.tun?.mut}`;

// A state from the radio, had at `at`. One that brings another station, from
// the radio's own knob, say, is drawn as a transition; the rest at once.
// One with no tuner part, as from a firmware that changed its shape, is
// dropped: no screen can draw it.
function take(s, at) {
  if (!s?.tun || at < newest) return;
  newest = at;
  const was = station(state.value);
  fromRadio = s;
  drawn = drawn.filter((d) => at < d.takenAt + SETTLE_MS);
  if (was && station(s) !== was) withMotion(show);
  else show();
}

// Reads the state now, as after a change, rather than at the next poll. The
// live state needs no asking: the next one comes within half a second.
async function poll() {
  if (live()) return;
  if (open) {
    again = true;
    return;
  }
  open = true;
  clearTimeout(timer);
  const started = now();
  try {
    take(await getJson('/api/state'), started);
    link.value = now() - started > 3000 ? 'slow' : 'ok';
  } catch {
    link.value = 'lost';
  }
  open = false;
  if (running && !live()) timer = setTimeout(poll, again ? 0 : POLL_MS);
  again = false;
}

// The live state: while the socket is open the radio sends its state and
// nothing is polled. Polling goes on while it opens, and takes over when it
// closes or goes quiet.
function openLive() {
  clearTimeout(retry);
  if (!running || socket) return;
  const ws = new WebSocket(`${location.protocol === 'https:' ? 'wss' : 'ws'}://${location.host}/api/live`);
  let opened = false;
  const heard = () => {
    clearTimeout(silence);
    silence = setTimeout(() => ended(ws, opened), SILENCE_MS);
  };
  socket = ws;
  // Timed from now, so a socket that hangs while it connects is closed too.
  heard();
  ws.onopen = () => {
    opened = true;
    retryMs = LIVE_RETRY_MS;
    clearTimeout(timer);
    link.value = 'ok';
    heard();
  };
  ws.onmessage = (e) => {
    heard();
    try {
      take(JSON.parse(e.data), now());
      link.value = 'ok';
    } catch {
      // Not a state: nothing else is sent on this socket.
    }
  };
  ws.onclose = () => ended(ws, opened);
}

function ended(ws, opened) {
  if (socket !== ws) return;
  socket = null;
  clearTimeout(silence);
  ws.close();
  if (!running) return;
  if (opened) {
    poll();
  } else {
    retryMs = Math.min(retryMs * 2, LIVE_RETRY_MAX_MS);
  }
  retry = setTimeout(openLive, opened ? LIVE_RETRY_MS : retryMs);
}

function closeLive() {
  clearTimeout(retry);
  clearTimeout(silence);
  const ws = socket;
  socket = null;
  ws?.close();
}

let queue = Promise.resolve();

// Sends a change, after any sent before it, so taps reach the radio in the
// order they were made. expect, when the result is known, draws it at once
// over the radio's last state, so the state that confirms it changes nothing
// on the screen. Without expect the state is read at once. A refusal takes
// the drawing back off, reads the state again, and throws as post does.
export function change(path, fields, expect) {
  let mark = null;
  if (expect && fromRadio) {
    mark = { patch: expect, takenAt: Infinity };
    drawn.push(mark);
    withMotion(show);
  }
  const sent = queue.then(async () => {
    let said;
    try {
      said = await post(path, fields);
    } catch (e) {
      if (mark) {
        drawn = drawn.filter((d) => d !== mark);
        withMotion(show);
      }
      poll();
      throw e;
    }
    if (mark) mark.takenAt = now();
    else poll();
    return said;
  });
  queue = sent.catch(() => {});
  return sent;
}

// The state while the page is in view; a hidden tab asks nothing of the
// radio and holds no socket.
export function startLive() {
  if (running) return;
  running = true;
  poll();
  openLive();
  checkSignIn();
  document.addEventListener('visibilitychange', () => {
    running = !document.hidden;
    clearTimeout(timer);
    if (running) {
      retryMs = LIVE_RETRY_MS;
      poll();
      openLive();
    } else {
      closeLive();
    }
  });
}
