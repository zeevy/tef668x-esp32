// The battery in the header, in a box like the ones beside it: a small
// battery on its side, filled to its level, and the level beside it in per
// cent or in volts, as the radio's own Battery setting says; with it Off
// there is none, as on the radio's screen. It shows the radio's newest
// reading: while Wi-Fi runs the radio cannot read it, so that is the one
// taken at start up, or, with Battery Check on, the one it paused Wi-Fi
// for. The label says how old.
import { duration } from '../data/format.js';
import { BATTERY_EMPTY_MV, BATTERY_FULL_MV } from '../data/limits.js';

// The Battery setting's values, as `bat.shw` gives them.
const SHOW_OFF = 0;
const SHOW_VOLTS = 2;

export function Battery({ bat }) {
  const mv = bat?.mv ?? bat?.last;
  if (!bat?.fit || !mv || bat.shw === SHOW_OFF) return null;
  const pc = Math.max(0, Math.min(100, Math.round(((mv - BATTERY_EMPTY_MV) * 100) / (BATTERY_FULL_MV - BATTERY_EMPTY_MV))));
  // Orange at a fifth left, red at a tenth.
  const fill = pc <= 10 ? 'bg-bad' : pc <= 20 ? 'bg-warn' : 'bg-accent/70';
  const when = bat.mv ? '' : bat.age < 60 ? ', read just now' : `, read ${duration(bat.age)} ago`;
  return (
    <span role="img" title={`Battery ${pc} %, ${(mv / 1000).toFixed(2)} V${when}`} aria-label={`Battery ${pc} per cent, ${(mv / 1000).toFixed(2)} volts${when}`} class="flex h-9 shrink-0 items-center gap-1 rounded-ctl border border-line bg-surface px-2 max-[379px]:px-1">
      <span class="flex items-center">
        <span class="relative h-3 w-[22px] overflow-hidden rounded-[3px] border-[1.5px] border-muted">
          <span class={`absolute inset-y-0 left-0 ${fill}`} style={{ width: `${pc}%` }} />
        </span>
        <span class="h-1.5 w-[2px] rounded-r-sm bg-muted" />
      </span>
      {/* Volts with no unit, as on the radio: the battery says what it is. */}
      <span class="num text-sm font-semibold max-[379px]:text-xs">{bat.shw === SHOW_VOLTS ? (mv / 1000).toFixed(1) : `${pc}%`}</span>
    </span>
  );
}
