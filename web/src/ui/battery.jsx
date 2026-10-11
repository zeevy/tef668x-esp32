// The battery in the header, as tall as the boxes beside it: an upright
// outline, filled from the foot to its level, with the level written along
// it in per cent or in volts. A tap switches between the two, and the
// browser remembers which. It shows the reading taken now when the radio has
// one; while Wi-Fi runs the radio cannot read it, and holds the one taken at
// start up, which the label then says.
import { useState } from 'preact/hooks';
import { BATTERY_EMPTY_MV, BATTERY_FULL_MV } from '../data/limits.js';
import { keep, load } from './theme.js';

const KEPT = 'battery-volts';

export function Battery({ bat }) {
  const [volts, setVolts] = useState(() => load(KEPT) === '1');
  const mv = bat?.mv ?? bat?.boot;
  if (!bat?.fit || !mv) return null;
  const pc = Math.max(0, Math.min(100, Math.round(((mv - BATTERY_EMPTY_MV) * 100) / (BATTERY_FULL_MV - BATTERY_EMPTY_MV))));
  // Orange at a fifth left, red at a tenth.
  const fill = pc <= 10 ? 'bg-bad/40' : pc <= 20 ? 'bg-warn/40' : 'bg-accent/40';
  const flip = () => {
    setVolts(!volts);
    keep(KEPT, volts ? '0' : '1');
  };
  const when = bat.mv ? '' : ', read when the radio started';
  return (
    <button type="button" onClick={flip} title={`Battery ${pc} %, ${(mv / 1000).toFixed(2)} V${when}. Tap for ${volts ? 'per cent' : 'volts'}.`} aria-label={`Battery ${pc} per cent, ${(mv / 1000).toFixed(2)} volts${when}`} class="hit relative flex h-9 flex-col items-center">
      <span class="h-[3px] w-2.5 rounded-t-sm bg-muted" />
      <span class="relative flex w-6 flex-1 items-center justify-center overflow-hidden rounded-[5px] border-2 border-muted text-ink">
        <span class={`absolute inset-x-0 bottom-0 ${fill}`} style={{ height: `${pc}%` }} />
        {/* Along the battery's length, read from the foot up. Volts with no
            unit, as on the radio: the battery says what it is. */}
        <span class="num relative rotate-180 text-xs font-semibold [writing-mode:vertical-rl]">{volts ? (mv / 1000).toFixed(1) : `${pc}%`}</span>
      </span>
    </button>
  );
}
