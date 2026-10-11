// A sweep of the band as a chart, for DX mode's Scope and the band scope: a
// bar a channel, the floor, the dial and the radio's cursor, and for DX the
// baseline, the peaks and the rise over the baseline. A tap tunes there; so
// does Enter, after Left and Right pick a channel.
import { useEffect, useState } from 'preact/hooks';
import { dbuv, freq, unit, when } from '../data/format.js';
import { Button } from './kit.jsx';

// The scale, as the radio's own scope draws it: from 3 dB under the floor,
// or -10 dBµV with none, 64 dB tall; the rise strip 10 dB each way. Every
// level here, the floor too, is in tenths of a dBµV.
const UNDER_FLOOR = 30;
const SCALE = 640;
const NO_FLOOR_BOTTOM = -100;
const RISE_RANGE = 100;
// The longest the chart stays dim for a sweep asked for that never arrives,
// one abandoned by a tune, say. The whole MW band takes about 7.5 s.
const SWEEP_WAIT_MS = 15000;

// The bars of one series as one path, a bar a channel 0.8 wide, so a sweep
// of hundreds of channels is a few elements and not hundreds. `bar` gives a
// channel's [top, height], or nothing for no bar.
const round = (x) => Math.round(x * 100) / 100;
function bars(values, bar) {
  let d = '';
  values?.forEach((v, i) => {
    const b = v != null && bar(v, i);
    if (b && b[1] > 0) d += `M${i} ${round(b[0])}h0.8v${round(b[1])}h-0.8z`;
  });
  return d;
}

// Where a level sits on a scale of 0 at the top to 100 at the foot.
export function levelY(sweep, v) {
  const bottom = sweep.floor != null ? sweep.floor - UNDER_FLOOR : NO_FLOOR_BOTTOM;
  return 100 - Math.max(0, Math.min(1, (v - bottom) / SCALE)) * 100;
}

// How long ago a sweep was taken.
export function age(sweep) {
  if (!sweep?.real) return '';
  const min = Math.round((Date.now() / 1000 - sweep.time) / 60);
  return min < 1 ? 'now' : min < 60 ? `${min} min ago` : when(sweep.time);
}

// Whether a sweep this page asked for is still on its way: from the moment
// the radio took the ask until a sweep newer than `rev` comes. The radio
// answers nothing while it sweeps, so this is how the page knows.
export function useAsked(rev) {
  const [over, setOver] = useState(null);
  useEffect(() => {
    if (over === null) return;
    const timer = setTimeout(() => setOver(null), SWEEP_WAIT_MS);
    return () => clearTimeout(timer);
  }, [over]);
  return [over !== null && (rev ?? 0) === over, () => setOver(rev ?? 0)];
}

export function SweepButton({ sweeping, kind = 'secondary', onClick }) {
  return (
    <Button kind={kind} icon="refresh-cw" onClick={onClick}>
      {sweeping ? 'Sweeping' : 'Sweep'}
    </Button>
  );
}

// `marks`: channels to point out along the top, { khz, kind: 'preset' or
// 'catch', label }. `cursorKhz`: the radio's cursor, or null.
export function SweepChart({ sweep, band, dialKhz, cursorKhz, dim, marks = [], onTune }) {
  // The channel pointed at, or picked with Left and Right.
  const [hover, setHover] = useState(null);
  const n = sweep.count;
  const y = (v) => levelY(sweep, v);
  const khzAt = (i) => sweep.low + i * sweep.step;
  const channel = (khz) => (khz != null && khz >= sweep.low ? Math.round((khz - sweep.low) / sweep.step) : -1);
  // The tuned channel, when it is on the chart.
  const dial = channel(dialKhz);
  const onChart = dial >= 0 && dial < n;
  // The radio's own cursor, which its knob moves, while its panel shows the
  // scope; the line under the chart follows it there, as the panel's does.
  const cursor = channel(cursorKhz);
  const cursorOn = cursor >= 0 && cursor < n;
  // The channel the line under the chart names: the one pointed at, else the
  // radio's cursor, else the dial.
  const at = hover ?? (cursorOn ? cursor : onChart ? dial : -1);
  const indexAt = (e) => {
    const box = e.currentTarget.getBoundingClientRect();
    return Math.min(n - 1, Math.max(0, Math.floor(((e.clientX - box.left) / box.width) * n)));
  };
  const named = marks.filter((m) => channel(m.khz) === at);
  // The line under the chart, while `at` is a channel: the channel, then its
  // readings and names.
  const where = `${freq(khzAt(at), band)} ${unit(band)}`;
  const about = [
    sweep.level[at] != null ? `${dbuv(sweep.level[at])} dBµV` : 'no reading',
    sweep.rise?.[at] != null && `${sweep.rise[at] >= 0 ? '+' : ''}${dbuv(sweep.rise[at])} dB over its baseline`,
    ...named.map((m) => m.label),
    at === cursor && at !== dial && hover === null && "the radio's cursor",
  ]
    .filter(Boolean)
    .map((x) => ` · ${x}`)
    .join('');
  function onKey(e) {
    const step = { ArrowLeft: -1, ArrowRight: 1 }[e.key];
    if (step) {
      e.preventDefault();
      setHover(Math.min(n - 1, Math.max(0, at < 0 ? 0 : at + step)));
    } else if (e.key === 'Enter' && at >= 0) {
      e.preventDefault();
      onTune(khzAt(at));
    }
  }
  const dialLevel = onChart ? sweep.level[dial] : null;
  const riseH = (v) => (Math.min(RISE_RANGE, Math.abs(v)) / RISE_RANGE) * 10;
  return (
    // Dim while a new sweep is on its way, so the old one is not taken for it.
    // A slider for the keys: Left and Right pick a channel, its readings
    // are the value a screen reader says, and Enter tunes there.
    <div
      class={`cursor-pointer touch-pan-y select-none transition-opacity ${dim ? 'opacity-30' : ''}`}
      onClick={(e) => onTune(khzAt(indexAt(e)))}
      onPointerMove={(e) => setHover(indexAt(e))}
      onPointerLeave={() => setHover(null)}
      onKeyDown={onKey}
      onBlur={() => setHover(null)}
      tabIndex={0}
      role="slider"
      aria-label="Pick a channel, Enter tunes to it"
      aria-valuemin={sweep.low}
      aria-valuemax={khzAt(n - 1)}
      aria-valuenow={at >= 0 ? khzAt(at) : undefined}
      aria-valuetext={at >= 0 ? where + about : 'The dial is off this sweep.'}
      aria-busy={dim}
    >
      <svg viewBox={`0 0 ${n} 100`} preserveAspectRatio="none" class="h-40 w-full md:h-56" role="img" aria-label="Level of every channel">
        {onChart && <rect x={dial - 0.6} y="0" width="2" height="100" class="fill-ink opacity-15" />}
        <path data-mark="levels" d={bars(sweep.level, (v, i) => i !== dial && [y(v), 100 - y(v)])} class="fill-accent" />
        {dialLevel != null && <rect x={dial} y={y(dialLevel)} width="0.8" height={100 - y(dialLevel)} class="fill-ink" />}
        {sweep.baseline_level && <path d={bars(sweep.baseline_level, (v) => [y(v) - 0.5, 1])} class="fill-ink" />}
        {sweep.peak && <path d={bars(sweep.peak, (v) => [y(v) - 0.5, 1])} class="fill-warn" />}
        {marks.map((m) => {
          const i = channel(m.khz);
          if (i < 0 || i >= n) return null;
          return m.kind === 'catch' ? <rect data-mark="catch" x={i - 0.2} y="0" width="1.2" height="4" class="fill-good" /> : <rect data-mark="preset" x={i + 0.2} y="0" width="0.4" height="4" class="fill-ink" />;
        })}
        {cursorOn && <line data-mark="cursor" x1={cursor + 0.4} x2={cursor + 0.4} y1="0" y2="100" class="stroke-good" stroke-width="2" vector-effect="non-scaling-stroke" />}
        {sweep.floor != null && <line x1="0" x2={n} y1={y(sweep.floor)} y2={y(sweep.floor)} class="stroke-muted" stroke-dasharray="2 2" vector-effect="non-scaling-stroke" />}
      </svg>
      {sweep.rise && (
        <svg viewBox={`0 0 ${n} 20`} preserveAspectRatio="none" class="mt-1 h-6 w-full rounded-ctl bg-raised" role="img" aria-label="Rise over the baseline">
          <path d={bars(sweep.rise, (v) => v > 0 && [10 - riseH(v), riseH(v)])} class="fill-accent" />
          <path d={bars(sweep.rise, (v) => v <= 0 && [10, riseH(v)])} class="fill-muted" />
          {cursorOn && <line x1={cursor + 0.4} x2={cursor + 0.4} y1="0" y2="20" class="stroke-good" stroke-width="2" vector-effect="non-scaling-stroke" />}
        </svg>
      )}
      <div class="num mt-1 flex justify-between text-xs text-muted">
        <span>{freq(sweep.low, band)}</span>
        <span>{freq(khzAt(Math.floor(n / 2)), band)}</span>
        <span>
          {freq(khzAt(n - 1), band)} {unit(band)}
        </span>
      </div>
      <p class="num mt-2 text-sm">
        {at >= 0 ? (
          <>
            <span class="font-semibold">{where}</span>
            <span class="text-muted">{about}</span>
          </>
        ) : (
          <span class="text-muted">The dial is off this sweep.</span>
        )}
      </p>
    </div>
  );
}
