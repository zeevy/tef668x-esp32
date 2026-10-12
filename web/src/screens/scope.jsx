// Band scope: the level of every channel of the band the radio is on, the
// whole band or a span round the dial; repeat; a waterfall of every sweep
// this page has seen; the sweep saved as a picture or a CSV. The station
// card with the tools on top; under it the chart, the waterfall lined up
// under it, and the facts.
import { useEffect, useRef, useState } from 'preact/hooks';
import { state } from '../data/radio.js';
import { catches, presets, scopeSweep } from '../data/reads.js';
import { dbuv, freq, isFm, level, presetTag, unit } from '../data/format.js';
import { act, say } from '../data/act.js';
import { logStation, tune } from '../data/commands.js';
import { Button, Card, Segmented, Switch, Unread } from '../ui/kit.jsx';
import { SweepButton, SweepChart, age, levelY, useAsked } from '../ui/sweep.jsx';
import { dark } from '../ui/theme.js';
import { StationNow, stationLabel } from '../ui/station.jsx';
import { Analyzer } from '../ui/backs.jsx';

// The span round the dial, as the radio's scope and tuning scale have it.
const SPAN_KHZ = { fm: 3600, am: 360 };
// The most sweeps the waterfall keeps.
const ROWS = 24;
// The waterfall's colours from the bottom of the scale to its top: the
// page's background, then dark red, orange and pale yellow, then the text
// colour, so each theme has a waterfall of its own.
const ramp = () => [
  [0, rgb('bg')],
  [0.45, [120, 40, 10]],
  [0.7, [232, 89, 12]],
  [0.88, [255, 200, 90]],
  [1, rgb('ink')],
];

// Every sweep of the same channels this page has seen, newest first, kept
// while the page is open, also across screens.
const seen = { key: '', sweeps: [] };
const keyOf = (s) => `${s.band}:${s.low}:${s.step}:${s.count}`;
function remember(sweep) {
  if (!sweep?.level) return;
  if (keyOf(sweep) !== seen.key) Object.assign(seen, { key: keyOf(sweep), sweeps: [] });
  if (seen.sweeps[0]?.rev === sweep.rev) return;
  seen.sweeps = [sweep, ...seen.sweeps].slice(0, ROWS);
}

export function BandScope() {
  const s = state.value;
  // The last sweep, read again only when the radio says there is a new one.
  const { data: latest, error } = scopeSweep.use();
  const presetList = presets.use().data;
  const caught = catches.use().data;
  const [shownRow, setShownRow] = useState(0);
  const [spanPicked, setSpanPicked] = useState(false);
  const [repeat, setRepeat] = useState(false);
  const running = s.scp?.run ?? false;
  const band = s.tun.bnd;
  const [asked, markAsked] = useAsked(latest?.rev);

  // Each new sweep goes on the waterfall and shows in the chart.
  remember(latest);
  useEffect(() => setShownRow(0), [latest?.rev]);

  // The presets of this band and, on FM and OIRT, the DX catches, marked
  // along the top.
  const marks = [
    ...(presetList ?? []).filter((p) => p.band === band).map((p) => ({ khz: p.khz, kind: 'preset', label: `${presetTag(p.slot)}${p.name?.trim() ? ` ${p.name.trim()}` : ''}` })),
    ...(isFm(band) ? (caught ?? []) : []).map((c) => ({ khz: c.khz, kind: 'catch', label: `caught ${stationLabel(c)}` })),
  ];

  // Repeat: the next sweep each time one ends. A sweep stopped on the radio
  // stops it, and so does leaving the page, since each sweep mutes the radio.
  // A whole sweep covers the band, or on SW the metre band the dial is in,
  // since the whole of shortwave is more channels than a sweep holds;
  // between two metre bands only a span fits.
  const wholeLabel = band !== 'SW' ? 'Whole band' : s.tun.mtb ? `${s.tun.mtb} band` : null;
  const wantWhole = wholeLabel !== null && !spanPicked;
  const spanKhz = wantWhole ? 0 : isFm(band) ? SPAN_KHZ.fm : SPAN_KHZ.am;
  async function sweepNow() {
    if (await act('/api/scope', { sweep: 1, span: spanKhz })) markAsked();
    else setRepeat(false);
  }
  const lastRev = useRef(null);
  useEffect(() => {
    if (!repeat || !latest || running || asked) return;
    if (latest.abandoned) {
      setRepeat(false);
      say('Repeat stopped: the sweep was stopped on the radio.', 'bad');
      return;
    }
    if (lastRev.current === latest.rev) return;
    lastRev.current = latest.rev;
    sweepNow();
  }, [repeat, latest?.rev, running, asked]);
  useEffect(() => {
    const hidden = () => document.hidden && setRepeat(false);
    document.addEventListener('visibilitychange', hidden);
    return () => document.removeEventListener('visibilitychange', hidden);
  }, []);

  const sweeping = running || asked;
  const sweep = seen.sweeps[shownRow] ?? (latest?.level ? latest : null);
  const onBand = sweep && sweep.band === band;
  const title = `${band} scope · ${sweeping ? 'sweeping' : sweep ? age(sweep) || 'no time' : 'no sweep yet'}`;

  const chart = sweep ? (
    <SweepChart sweep={sweep} band={sweep.band ?? band} dialKhz={onBand ? s.tun.khz : null} cursorKhz={onBand ? s.pnl?.cur : null} dim={sweeping} marks={onBand ? marks : []} onTune={tune} />
  ) : error ? (
    <Unread what="sweep" />
  ) : (
    <p class="text-muted">No sweep yet. Sweep the band to see every channel's level.</p>
  );
  const waterfall = sweep && <Waterfall sweeps={seen.sweeps} shown={shownRow} onPick={setShownRow} />;
  const facts = sweep && (
    <p class="num text-sm text-muted">
      {sweep.whole ? (sweep.band === 'SW' ? 'Metre band' : 'Whole band') : 'Span'} · {sweep.count} channels · width {sweep.width} kHz
      {sweep.floor != null && ` · floor ${level(sweep.floor)}`}
      {sweep.took_ms ? ` · took ${(sweep.took_ms / 1000).toFixed(1)} s` : ''}
      {shownRow > 0 && ` · an older sweep, ${shownRow} back`}
    </p>
  );

  // As Presets & Log and DX: the station on the left, the sweep's tools on
  // the right, the chart, its waterfall and its facts in a card under it.
  const repeatSwitch = (
    <Switch
      label="Repeat"
      on={repeat}
      onChange={(on) => {
        lastRev.current = latest?.rev ?? null;
        setRepeat(on);
        if (on && !sweeping) sweepNow();
      }}
    />
  );
  const what = wholeLabel && <Segmented label="What to sweep" options={[wholeLabel, 'Span']} value={wantWhole ? wholeLabel : 'Span'} onChange={(v) => setSpanPicked(v === 'Span')} />;
  const log = (
    <Button kind="secondary" icon="scroll-text" onClick={logStation}>
      Log
    </Button>
  );
  const png = (
    <Button kind="secondary" icon="download" onClick={() => sweep && savePng(sweep, band)}>
      PNG
    </Button>
  );
  const csv = (
    <Button kind="secondary" icon="download" onClick={() => sweep && saveCsv(sweep, band)}>
      CSV
    </Button>
  );
  const chartCard = (
    <Card title={title}>
      {chart}
      {waterfall && <div class="mt-2">{waterfall}</div>}
      {facts && <div class="mt-2">{facts}</div>}
      {repeat && <p class="mt-2 text-sm text-muted">Each sweep mutes the radio for its seconds.</p>}
    </Card>
  );
  return (
    <div class="grid grid-cols-1 gap-4">
      <StationNow s={s} back={<Analyzer />} sub={`${band} · ${sweep ? `${sweep.count} channels` : 'no sweep yet'}`}>
        <div class="grid grid-cols-2 gap-2">
          <SweepButton kind="primary" sweeping={sweeping} onClick={sweepNow} />
          {repeatSwitch}
        </div>
        <div class="grid grid-cols-3 gap-2">
          {log}
          {png}
          {csv}
        </div>
        {what}
      </StationNow>
      {chartCard}
    </div>
  );
}

// A level's colour from the ramp `r`, written straight into an image's
// bytes at `k`; no reading leaves the foot there. This runs for every pixel,
// so it makes no array of its own.
function rampAt(px, k, sweep, v, r) {
  if (v == null) return;
  const t = 1 - levelY(sweep, v) / 100;
  for (let j = 1; j < r.length; j++) {
    const [t1, c1] = r[j];
    const [t0, c0] = r[j - 1];
    if (t <= t1) {
      const f = (t - t0) / (t1 - t0);
      for (let ch = 0; ch < 3; ch++) px[k + ch] = Math.round(c0[ch] + (c1[ch] - c0[ch]) * f);
      return;
    }
  }
  px.set(r.at(-1)[1], k);
}

// The sweeps onto `canvas`, a pixel a channel and a row a sweep from the
// top, `n` channels wide and `rows` tall, on the ramp's foot with full
// opacity where there is no sweep or no reading.
function paintFall(canvas, sweeps, n, rows) {
  canvas.width = n;
  canvas.height = rows;
  const g = canvas.getContext('2d');
  const img = g.createImageData(n, rows);
  const r = ramp();
  const foot = [...r[0][1], 255];
  for (let k = 0; k < img.data.length; k += 4) img.data.set(foot, k);
  sweeps.forEach((sw, row) => sw.level.forEach((v, i) => rampAt(img.data, (row * n + i) * 4, sw, v, r)));
  g.putImageData(img, 0, 0);
}

// Every sweep seen, newest at the top, a row a sweep, further up the ramp
// where stronger, in a fixed area that fills from the top, ROWS sweeps
// tall. A tap on a row shows that sweep in the chart; Older and Newer step
// through them by keys, or by a finger too big for a row a few pixels tall.
function Waterfall({ sweeps, shown, onPick }) {
  const canvas = useRef(null);
  const isDark = dark();
  // Drawn again only when a sweep comes, the list a new array then, or the
  // theme changes.
  useEffect(() => {
    if (canvas.current && sweeps.length) paintFall(canvas.current, sweeps, sweeps[0].count, ROWS);
  }, [sweeps, isDark]);
  return (
    <div>
      {/* Its own box, so the mark on the row shown is placed by the
          picture's height alone. */}
      <div class="relative">
        <canvas
          ref={canvas}
          role="img"
          aria-label={`Waterfall of ${sweeps.length} sweeps`}
          class="h-24 w-full cursor-pointer rounded-ctl [image-rendering:pixelated] md:h-32"
          onClick={(e) => {
            const box = e.currentTarget.getBoundingClientRect();
            const row = Math.floor(((e.clientY - box.top) / box.height) * ROWS);
            if (row < sweeps.length) onPick(row);
          }}
        />
        {shown > 0 && (
          <div class="pointer-events-none absolute inset-x-0 bg-ink/60" style={{ top: `${(shown / ROWS) * 100}%`, height: `${100 / ROWS}%` }} />
        )}
      </div>
      <div class="mt-1 flex flex-wrap items-center gap-2">
        <p class="text-xs text-muted">
          {sweeps.length === 1 ? '1 sweep. Each new sweep adds a row at the top.' : `${sweeps.length} sweeps, newest at the top. Tap a row to see that sweep.`}
        </p>
        {sweeps.length > 1 && (
          <div class="ml-auto flex gap-2">
            <Button kind="secondary" aria-label="Older sweep" disabled={shown >= sweeps.length - 1} onClick={() => onPick(shown + 1)}>
              Older
            </Button>
            <Button kind="secondary" aria-label="Newer sweep" disabled={shown === 0} onClick={() => onPick(shown - 1)}>
              Newer
            </Button>
          </div>
        )}
      </div>
    </div>
  );
}

// A theme colour as the browser has it now: as CSS, and as [r, g, b].
function colour(name) {
  const el = document.createElement('span');
  el.style.color = `var(--${name})`;
  document.body.append(el);
  const c = getComputedStyle(el).color;
  el.remove();
  return c;
}
const rgb = (name) => colour(name).match(/\d+/g).slice(0, 3).map(Number);

function stamp(sweep) {
  const d = sweep.real ? new Date(sweep.time * 1000) : new Date();
  const two = (x) => String(x).padStart(2, '0');
  return `${d.getFullYear()}${two(d.getMonth() + 1)}${two(d.getDate())}-${two(d.getHours())}${two(d.getMinutes())}`;
}

function save(blob, name) {
  const a = document.createElement('a');
  a.href = URL.createObjectURL(blob);
  a.download = name;
  a.click();
  setTimeout(() => URL.revokeObjectURL(a.href), 1000);
}

// The sweep in the chart: each frequency in kHz and its level in dBµV.
function saveCsv(sweep, band) {
  const lines = ['khz,dbuv', ...sweep.level.map((v, i) => `${sweep.low + i * sweep.step},${v == null ? '' : dbuv(v)}`)];
  save(new Blob([lines.join('\n') + '\n'], { type: 'text/csv' }), `scope-${band}-${stamp(sweep)}.csv`);
}

// The chart, the waterfall and a line saying what it is, as one picture.
function savePng(sweep, band) {
  const w = 1200;
  const chartH = 300;
  const rows = seen.sweeps.length;
  const fallH = rows * 4;
  const c = document.createElement('canvas');
  c.width = w;
  c.height = chartH + fallH + 40;
  const g = c.getContext('2d');
  g.fillStyle = colour('surface');
  g.fillRect(0, 0, c.width, c.height);
  const bw = w / sweep.count;
  g.fillStyle = colour('accent');
  sweep.level.forEach((v, i) => {
    if (v == null) return;
    const y = (levelY(sweep, v) / 100) * chartH;
    g.fillRect(i * bw, y, Math.max(1, bw - 1), chartH - y);
  });
  if (rows > 0) {
    const fall = document.createElement('canvas');
    paintFall(fall, seen.sweeps, sweep.count, rows);
    g.imageSmoothingEnabled = false;
    g.drawImage(fall, 0, chartH, w, fallH);
  }
  g.fillStyle = colour('ink');
  g.font = `16px ${getComputedStyle(document.body).fontFamily}`;
  const top = sweep.low + (sweep.count - 1) * sweep.step;
  g.fillText(`${band} ${freq(sweep.low, band)} to ${freq(top, band)} ${unit(band)} · width ${sweep.width} kHz · ${sweep.real ? new Date(sweep.time * 1000).toLocaleString() : 'no time'}`, 8, chartH + fallH + 26);
  c.toBlob((b) => b && save(b, `scope-${band}-${stamp(sweep)}.png`), 'image/png');
}
