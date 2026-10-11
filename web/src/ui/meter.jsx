// The DX meter: the level now with the highest and the average since the
// last tune, a bar with the two marked on it, and the level of the last
// minute as a graph, each reading coloured for mono, stereo or RDS.
import { useRef } from 'preact/hooks';
import { step } from '../data/commands.js';
import { inUnit, levelUnit, nextUnit } from '../data/format.js';
import { useQuiet } from './kit.jsx';

// How far back the graph goes, and the top of the bar and the graph, 80
// dBµV, in tenths.
const HISTORY_MS = 60000;
const TOP = 800;

// Each reading kept once, as the radio's frame brings it: each frame is a
// new state, so a render with no new frame adds nothing, and a state the
// page drew itself, such as a tune shown before the radio takes it, holds
// the last frame's readings, so it adds nothing either. Started again on
// every tune.
export function useMeter(s) {
  const c = useRef({ khz: null }).current;
  const t = s?.tun;
  if (t && c.khz !== t.khz) Object.assign(c, { khz: t.khz, list: [], peak: null, sum: 0, n: 0, last: null });
  if (t && s !== c.last && !s.patched && t.sig != null) {
    c.last = s;
    const now = performance.now();
    const kind = t.rds?.syn ? 'rds' : t.st && !t.mno ? 'st' : 'mono';
    c.list = [...c.list.filter((r) => now - r.at < HISTORY_MS), { at: now, v: t.sig, kind }];
    c.peak = Math.max(c.peak ?? t.sig, t.sig);
    c.sum += t.sig;
    c.n++;
  }
  return { list: c.list ?? [], now: t?.sig, peak: c.peak, avg: c.n ? c.sum / c.n : null };
}

const KIND = { mono: 'text-muted', st: 'text-accent', rds: 'text-good' };
const pct = (v) => `${Math.max(0, Math.min(100, (v / TOP) * 100))}%`;

// A channel a wheel notch, at most one each 100 ms, as FM-DX Webserver.
// The notches add up and go to the radio as one step once the wheel is
// still, as the arrow keys do, so the dial does not go on moving after. A
// sideways swipe steps nothing.
function useWheelStep() {
  const last = useRef(0);
  const steps = useQuiet((n) => n && step(n));
  return (e) => {
    if (!e.deltaY) return;
    e.preventDefault();
    const now = performance.now();
    if (now - last.current < 100) return;
    last.current = now;
    steps.later((steps.value ?? 0) + (e.deltaY < 0 ? 1 : -1));
  };
}

// The readings in runs of the same kind, each run starting where the last
// ended, so the line has no gap where its colour changes.
function runs(list) {
  const out = [];
  list.forEach((r, i) => {
    if (i === 0 || r.kind !== list[i - 1].kind) out.push({ kind: r.kind, points: i ? [list[i - 1]] : [] });
    out.at(-1).points.push(r);
  });
  return out;
}

// The level of the last minute as a line with the area under it filled,
// where each reading fell in time, now at the right. The wheel over it
// tunes.
function LevelGraph({ list }) {
  const wheel = useWheelStep();
  const now = list.at(-1)?.at ?? 0;
  const x = (at) => ((HISTORY_MS - (now - at)) / 100).toFixed(1);
  const y = (v) => (48 - Math.max(0, Math.min(1, v / TOP)) * 48).toFixed(1);
  return (
    <svg viewBox={`0 0 ${HISTORY_MS / 100} 48`} preserveAspectRatio="none" onWheel={wheel} class="h-10 w-full rounded-ctl bg-raised/50" role="img" aria-label="Level, last minute">
      {runs(list).map(({ kind, points }) => {
        const line = points.map((r) => `${x(r.at)},${y(r.v)}`).join(' ');
        return (
          <g class={KIND[kind]}>
            <polygon points={`${x(points[0].at)},48 ${line} ${x(points.at(-1).at)},48`} class="fill-current opacity-25" />
            <polyline points={line} fill="none" stroke="currentColor" stroke-width="1.5" stroke-linejoin="round" vector-effect="non-scaling-stroke" />
          </g>
        );
      })}
    </svg>
  );
}

const SWATCH = [
  ['bg-muted', 'Mono'],
  ['bg-accent', 'Stereo'],
  ['bg-good', 'RDS'],
];

export function DxMeter({ m }) {
  return (
    <div class="grid gap-2 rounded-ctl bg-raised/60 p-2">
      <div class="flex flex-wrap items-baseline justify-between gap-x-2">
        <button type="button" onClick={nextUnit} title="Tap for dBµV, dBf or dBm" class="hit relative flex items-baseline gap-1 text-left">
          <span class="num text-4xl leading-none font-semibold">{inUnit(m.now)}</span>
          <span class="text-sm text-muted">{levelUnit.value}</span>
        </button>
        <span class="num text-sm whitespace-nowrap text-muted">
          Peak {inUnit(m.peak)} · Avg {inUnit(m.avg)}
        </span>
      </div>
      <div class="relative h-3 rounded-ctl bg-surface">
        <div class="h-3 rounded-ctl bg-accent" style={{ width: pct(m.now ?? 0) }} />
        {m.avg != null && <i class="absolute top-0 h-3 w-0.5 bg-ink/60" style={{ left: pct(m.avg) }} title="Average" />}
        {m.peak != null && <i class="absolute -top-1 h-5 w-0.5 bg-warn" style={{ left: pct(m.peak) }} title="Peak" />}
      </div>
      <LevelGraph list={m.list} />
      <span class="flex gap-4 text-xs text-muted">
        {SWATCH.map(([bg, name]) => (
          <span class="flex items-center gap-1">
            <i class={`size-2 rounded-sm ${bg}`} />
            {name}
          </span>
        ))}
      </span>
    </div>
  );
}
