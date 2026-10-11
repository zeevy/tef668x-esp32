// How the page writes the radio's numbers.
import { signal } from '@preact/signals';
import { keep, load } from './store.js';

// The radio's bands. FM and OIRT are FM, in MHz; LW, MW and SW are AM, in kHz.
export const BANDS = ['FM', 'OIRT', 'LW', 'MW', 'SW'];
export const isFm = (band) => band === 'FM' || band === 'OIRT';

// The frequency of a band in its own unit: 106.40 on FM, 738 on MW.
export function freq(khz, band) {
  return isFm(band) ? (khz / 1000).toFixed(2) : String(khz);
}

export function unit(band) {
  return isFm(band) ? 'MHz' : 'kHz';
}

// A preset's number as the radio writes it: P09.
export const presetTag = (n) => `P${String(n).padStart(2, '0')}`;

// A level the radio sends in tenths of a dBuV.
export function dbuv(tenths) {
  return (tenths / 10).toFixed(1);
}

// The unit this browser shows levels in: dBµV, the radio's own, or dBf or
// dBm as XDR-GTK and FM-DX Webserver give them, dBf = dBµV + 11.25 and
// dBm = dBf - 120. The shift is in tenths. Settings and the CSV files stay
// in dBµV, as the radio keeps them.
const SHIFT = { dBµV: 0, dBf: 112.5, dBm: -1087.5 };
const UNITS = Object.keys(SHIFT);
export const levelUnit = signal(UNITS.includes(load('unit')) ? load('unit') : 'dBµV');
export function nextUnit() {
  levelUnit.value = UNITS[(UNITS.indexOf(levelUnit.value) + 1) % UNITS.length];
  keep('unit', levelUnit.value);
}

// A level in that unit, the number alone, or with its unit.
export const inUnit = (tenths) => (tenths == null ? '--' : ((tenths + SHIFT[levelUnit.value]) / 10).toFixed(1));
export const level = (tenths) => `${inUnit(tenths)} ${levelUnit.value}`;

// A station's readings, from the state's `tun` or from a log entry, each as
// { label, value, unit }, with `max` for a bar. The noise is an FM reading,
// as on the radio's own screens, so it is null on AM; there the co-channel
// reading takes the multipath reading's place. With no level, as before the
// tuner's first reading, every value is --.
export function readings(band, { sig, snr, usn, wam, off, bw, mod }) {
  const fm = isFm(band);
  const pc = (tenths) => (tenths / 10).toFixed(0);
  const r = {
    level: { label: 'Level', value: inUnit(sig), unit: levelUnit.value, pct: (sig / 800) * 100 },
    snr: { label: 'SNR', value: snr, unit: 'dB', max: 40 },
    noise: fm ? { label: 'Noise', value: pc(usn), unit: '%', max: 100 } : null,
    wam: { label: fm ? 'Multipath' : 'Co-channel', value: pc(wam), unit: '%', max: 100 },
    width: { label: 'Width', value: bw, unit: 'kHz' },
    offset: { label: 'Offset', value: (off / 10).toFixed(1), unit: 'kHz' },
    mod: { label: 'Modulation', value: mod, unit: '%' },
  };
  if (sig == null) for (const f of Object.values(r)) if (f) f.value = '--';
  return r;
}

// A text the radio keeps: printable ASCII, `max` characters at most.
export const printable = (text, max) => text.replace(/[^\x20-\x7e]/g, '').slice(0, max);

// How long, from seconds: 42 min, 3 h 5 min, 2 d 4 h.
export function duration(s) {
  const m = Math.floor(s / 60);
  if (m < 60) return `${m} min`;
  const h = Math.floor(m / 60);
  if (h < 24) return `${h} h ${m % 60} min`;
  return `${Math.floor(h / 24)} d ${h % 24} h`;
}

// A log time: the clock time today, else the day and month.
export function when(epoch, now = Date.now()) {
  const d = new Date(epoch * 1000);
  const today = new Date(now).toDateString() === d.toDateString();
  return today
    ? d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })
    : d.toLocaleDateString([], { day: 'numeric', month: 'short' });
}

// The date as the radio's own screen writes it: FRIDAY, 10th October 2026.
export function dateLine(d) {
  const day = d.getDate();
  const suffix = day >= 11 && day <= 13 ? 'th' : ({ 1: 'st', 2: 'nd', 3: 'rd' }[day % 10] ?? 'th');
  const name = (part) => d.toLocaleDateString('en-GB', { [part]: 'long' });
  return `${name('weekday').toUpperCase()}, ${day}${suffix} ${name('month')} ${d.getFullYear()}`;
}

// A typed frequency in kHz. With a point, or under 144, it is MHz: no AM
// band starts below 144 kHz, and FM and OIRT are 65 to 108 MHz. So 98.3 and
// 98 are 98300, 9.65 is 9650; 738, 153 and 98300 are kHz. Given an FM
// band, three or more digits with no point are MHz with the point put in, as
// FM-DX Webserver reads them: 875 is 87.5, 1043 is 104.3, 10435 is 104.35.
export function typedKhz(text, band) {
  let t = String(text).trim();
  if (isFm(band) && /^\d{3,5}$/.test(t)) t = t.startsWith('1') ? `${t.slice(0, 3)}.${t.slice(3)}` : `${t.slice(0, 2)}.${t.slice(2)}`;
  const v = Number(t.replace(',', '.'));
  if (t === '' || !Number.isFinite(v) || v <= 0) return null;
  return /[.,]/.test(t) || v < 144 ? Math.round(v * 1000) : Math.round(v);
}
