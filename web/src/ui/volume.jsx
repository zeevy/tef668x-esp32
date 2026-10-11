// The volume controls: the slider Home and the header share, and the box
// in the header with the mute key, the slider and the volume.
import { volHeld } from '../data/radio.js';
import { mute, volume } from '../data/commands.js';
import { VOL_MAX, VOL_MIN } from '../data/limits.js';
import { Icon, Slider, useQuiet } from './kit.jsx';

// The volume as a slider, drawn on every volume control as the finger or
// the keys move it, and sent once it is still.
export function VolumeSlider({ tun }) {
  const quiet = useQuiet(volume);
  return (
    <Slider
      label="Volume"
      min={VOL_MIN}
      max={VOL_MAX}
      value={volHeld.value ?? tun.vol}
      onInput={(v) => {
        volHeld.value = v;
        quiet.later(v);
      }}
    />
  );
}

function MuteKey({ tun }) {
  const label = tun.mut ? 'Sound on' : 'Mute';
  return (
    <button
      type="button"
      aria-label={label}
      title={label}
      onClick={() => mute(!tun.mut)}
      class={`hit relative grid size-7 shrink-0 place-items-center rounded-ctl hover:bg-raised ${tun.mut ? 'text-warn' : ''}`}
    >
      <Icon name={tun.mut ? 'volume-x' : 'volume-2'} size={18} />
    </button>
  );
}

// The mute key, the slider and the volume with its unit, in a box as the
// frequency beside it has. The number keeps one width, so nothing moves as
// it changes.
export function VolumeBox({ tun, class: cls = '' }) {
  return (
    <span class={`h-9 items-center gap-2 rounded-ctl border border-line bg-surface px-2 ${cls}`}>
      <MuteKey tun={tun} />
      <span class="range-slim min-w-0 flex-1 md:w-28 md:flex-none">
        <VolumeSlider tun={tun} />
      </span>
      <span class="flex w-12 shrink-0 items-baseline justify-end gap-1">
        <span class="num">{volHeld.value ?? tun.vol}</span>
        <span class="text-xs text-muted">dB</span>
      </span>
    </span>
  );
}
