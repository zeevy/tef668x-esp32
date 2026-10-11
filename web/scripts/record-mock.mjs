// Records what the radio's API answers, into mock/, for npm run dev.
//
//   npm run record                  the radio at http://tef668x.lan:8080
//   RADIO=http://192.168.1.40:8080 TEF_PIN=123456 npm run record
//
// Only GET routes are read: nothing on the radio changes. The recordings go
// into a public repository, so before a file is saved the home network's
// name, the hotspot's name, the PC Link's PC addresses and every log note
// are blanked, and every IPv4 address is replaced by a stand-in.
import fs from 'node:fs';
import path from 'node:path';

const RADIO = process.env.RADIO || 'http://tef668x.lan:8080';
const PIN = process.env.TEF_PIN || '000000';

// Path, file, content type, and whether it needs the PIN.
const ROUTES = [
  ['/api/state', 'state.json', 'application/json', false],
  ['/api/bands', 'bands.json', 'application/json', false],
  ['/api/settings', 'settings.json', 'application/json', true],
  ['/api/dx', 'dx.ndjson', 'application/x-ndjson', false],
  ['/api/dx/sweep', 'dx-sweep.json', 'application/json', false],
  ['/api/scope', 'scope.json', 'application/json', false],
  ['/api/presets.csv', 'presets.csv', 'text/csv', false],
  ['/api/log', 'log.ndjson', 'application/x-ndjson', false],
  ['/api/log.csv', 'log.csv', 'text/csv', false],
  ['/api/dx.csv', 'dx.csv', 'text/csv', false],
];

// What each private field becomes, wherever it is in a JSON answer. A field
// that is empty or null is kept as it is.
const BLANK = { sid: 'MyNetwork', apn: 'tef668x-setup-A1B2', ips: [], note: null };
const blank = (key, value) => (Object.hasOwn(BLANK, key) && value ? BLANK[key] : value);

// The fields of one CSV line. A field in double quotes can hold commas and
// doubled quotes; the radio never puts a line break in a field.
function csvFields(line) {
  const field = /"(?:[^"]|"")*"|[^,]*/y;
  const out = [];
  for (let at = 0; ; at++) {
    field.lastIndex = at;
    out.push(field.exec(line)[0]);
    at += out.at(-1).length;
    if (at >= line.length) return out;
  }
}

// Each IPv4 address gets its own stand-in, the same one in every file: the
// radio's own address is 192.168.1.40, the next one found 192.168.1.41, and
// so on.
const standIns = new Map();
function hideAddresses(text) {
  return text.replace(/\b(?:\d{1,3}\.){3}\d{1,3}\b/g, (ip) => {
    if (!standIns.has(ip)) standIns.set(ip, `192.168.1.${40 + standIns.size}`);
    return standIns.get(ip);
  });
}

// A JSON answer is read and written again with its private fields blanked,
// an NDJSON answer one line at a time. A CSV answer has its note column
// emptied. Text that does not parse stops the recording, so nothing is
// saved half hidden.
function scrub(text, type) {
  const lines = text.split('\n');
  if (type === 'application/json') {
    text = JSON.stringify(JSON.parse(text, blank));
  } else if (type === 'application/x-ndjson') {
    text = lines.map((l) => (l.trim() ? JSON.stringify(JSON.parse(l, blank)) : l)).join('\n');
  } else if (type === 'text/csv') {
    const note = csvFields(lines[0]).indexOf('note');
    if (note >= 0) {
      text = lines.map((l, i) => (i && l ? csvFields(l).map((f, j) => (j === note ? '' : f)).join(',') : l)).join('\n');
    }
  }
  return hideAddresses(text);
}

const auth = await fetch(`${RADIO}/auth`, {
  method: 'POST',
  body: new URLSearchParams({ pin: PIN }),
  redirect: 'manual',
});
const cookie = (auth.headers.get('set-cookie') || '').split(';')[0];
const state = await (await fetch(`${RADIO}/api/state`)).json();
if (state.ip) standIns.set(state.ip, '192.168.1.40');

const index = {};
for (const [route, file, type, pin] of ROUTES) {
  const res = await fetch(`${RADIO}${route}`, pin ? { headers: { cookie } } : {});
  if (!res.ok) {
    console.log(`${route}: ${res.status}, not recorded`);
    continue;
  }
  const body = scrub(await res.text(), type);
  fs.writeFileSync(path.join('mock', file), body);
  index[route] = { file, type };
  console.log(`${route}: ${body.length} bytes`);
}
fs.writeFileSync(path.join('mock', 'index.json'), JSON.stringify(index, null, 2) + '\n');
console.log('Read git diff web/mock before a commit: the recordings go into a public repository.');
