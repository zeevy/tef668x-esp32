// The signal level of the last 60 s, as the radio's DX page draws it: kept
// from the live frames, and drawn as a bar a reading where it fell in time.
import { useEffect, useRef } from 'preact/hooks';

// How far back the level history goes, and the top of its scale, 70 dBµV,
// in tenths.
const HISTORY_MS = 60000;
const TOP = 700;

// The signal level of the last 60 s, each reading with when it came, so
// the chart covers 60 s whether the frames come twice a second or every 5 s
// while polling. Started again on every tune.
export function useLevelHistory(t) {
  const list = useRef({ khz: null, levels: [] });
  if (t && list.current.khz !== t.khz) list.current = { khz: t.khz, levels: [] };
  useEffect(() => {
    if (!t) return;
    const now = performance.now();
    list.current.levels = [...list.current.levels.filter((r) => now - r.at < HISTORY_MS), { at: now, v: t.sig }];
  }, [t]);
  return list.current.levels;
}

// 60 s of the level, a bar a reading where it fell in time, now at the
// right, 0 to 70 dBµV, as the radio's DX page.
export function LevelHistory({ levels, class: cls }) {
  const now = levels.at(-1)?.at ?? 0;
  return (
    <div class="grid gap-1">
      <span class="label">Level, last 60 s</span>
      <svg viewBox={`0 0 ${HISTORY_MS / 100} 48`} preserveAspectRatio="none" class={`w-full rounded-ctl bg-raised ${cls}`} role="img" aria-label="Level, last 60 s">
        {levels.map(({ at, v }) => {
          const h = Math.max(0, Math.min(1, v / TOP)) * 48;
          return <rect x={(HISTORY_MS - (now - at)) / 100 - 4} y={48 - h} width="4" height={h} class="fill-accent" />;
        })}
      </svg>
    </div>
  );
}
