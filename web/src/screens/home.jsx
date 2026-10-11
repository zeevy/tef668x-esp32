// Home: what the radio is doing at a glance, the dial's buttons, the presets
// as a strip, the last stations heard, and the radio's own state.
import { useEffect, useRef, useState } from 'preact/hooks';
import { state, volHeld } from '../data/radio.js';
import { lastHeard, logFits, presets, scopeSweep } from '../data/reads.js';
import { dbuv, duration, readings, when } from '../data/format.js';
import { VolumeSlider } from '../ui/volume.jsx';
import { logStation, mute, recall, seek, step, storeStation, tune } from '../data/commands.js';
import { Spectrum } from '../ui/spectrum.jsx';
import { AddKey, KeysHelp, PresetKey, useKeys } from '../ui/keys.jsx';
import { Banner, Card, Figure, Icon, IconButton, IconTile, Loading, Meter, Unread } from '../ui/kit.jsx';
import { Frequency, StationChips, StationRow, StationText } from '../ui/station.jsx';
import { checkSays } from './system.jsx';

export function Home() {
  const s = state.value;
  const presetsRead = presets.use();
  const heard = lastHeard.use();
  const sweep = scopeSweep.use().data;
  const typedSlot = useKeys(presetsRead.data);
  const t = s.tun;
  const r = readings(t.bnd, t);
  const button = (icon, label, send, kind) => <IconButton icon={icon} label={label} kind={kind} class="w-full @md:w-11" onClick={send} />;
  const logThis = async () => (await logStation()) && lastHeard.again();
  return (
    <div class="grid grid-cols-1 gap-4">
      {s.dpn && (
        <Banner action={<a href="#network" class="hit relative font-semibold underline">Change</a>}>
          The radio still uses the default PIN.
        </Banner>
      )}
      <Card class="@container" back={<Spectrum t={t} sweep={sweep?.level ? sweep : null} />}>
        <StationChips s={s}>
          <span class="num ml-auto text-sm text-muted">{s.clk.now}</span>
        </StationChips>
        <div class="mt-4 grid grid-cols-1 gap-4 @xl:grid-cols-2">
          <div class="flex flex-col justify-between gap-4">
            <div>
              <Frequency t={t} />
              <StationText s={s} presets={presetsRead.data} />
            </div>
            <div class="grid grid-cols-6 gap-2 @md:flex">
              {button('skip-back', 'Seek down', () => seek(false))}
              {button('chevron-left', 'Step down', () => step(-1))}
              {button(t.mut ? 'volume-x' : 'volume-2', t.mut ? 'Sound on' : 'Mute', () => mute(!t.mut), 'primary')}
              {button('chevron-right', 'Step up', () => step(1))}
              {button('skip-forward', 'Seek up', () => seek(true))}
              <IconButton icon="scroll-text" label="Log this station" class="w-full @md:w-11" onClick={logThis} />
            </div>
          </div>
          {/* The bars together at the top, the readings at the foot, level with
              the buttons on the other half. */}
          <div class="flex flex-col justify-between gap-4">
            <div class="grid gap-4">
            <Meter {...r.level} />
            <Meter {...r.snr} label="Signal to noise" />
            <Meter label="Volume" value={volHeld.value ?? t.vol} unit="dB">
              <VolumeSlider tun={t} />
            </Meter>
            </div>
            <div class="flex justify-between gap-2">
              {[r.noise, r.wam, r.width, r.offset].filter(Boolean).map((f) => (
                <Figure {...f} />
              ))}
            </div>
          </div>
        </div>
      </Card>
      <div class="grid grid-cols-1 gap-4 lg:grid-cols-2">
        <Card class="@container grid content-center">
          <Facts s={s} />
        </Card>
        <Card title="Presets" end={<a href="#presets" class="hit relative text-sm text-accent-ink">All</a>}>
          <PresetStrip presets={presetsRead.data} error={presetsRead.error} current={t.pst} store={() => storeStation(t)} />
        </Card>
      </div>
      <Card title="Last heard" end={<a href="#log" class="hit relative text-sm text-accent-ink">Log</a>}>
        <LastHeard log={heard.data} error={heard.error} />
      </Card>
      <KeysHelp typedSlot={typedSlot} />
    </div>
  );
}

// Keys of one width, wide enough for the band, the number, the frequency and
// the name; the last one in view may be cut at the edge, which says there are
// more. A swipe stops with a key's edge at the start.
function PresetStrip({ presets, error, current, store }) {
  const strip = useRef();
  // Whether there are more keys out of view on each side, for the arrows.
  const [more, setMore] = useState({ left: false, right: false });
  const look = () => {
    const el = strip.current;
    if (el) setMore({ left: el.scrollLeft > 1, right: el.scrollLeft + el.clientWidth < el.scrollWidth - 1 });
  };
  // The preset on the dial second, the one before it in view: at once when
  // the page opens, and moved to when the dial goes to another preset.
  const opened = useRef(false);
  useEffect(() => {
    const keys = [...(strip.current?.children ?? [])];
    const at = keys.findIndex((k) => k.getAttribute('aria-pressed') === 'true');
    if (at >= 0) strip.current.scrollTo({ left: at > 0 ? keys[at - 1].offsetLeft : 0, behavior: opened.current ? 'smooth' : 'instant' });
    if (keys.length) opened.current = true;
    look();
  }, [presets, current]);
  useEffect(() => {
    addEventListener('resize', look);
    return () => removeEventListener('resize', look);
  }, []);
  if (!presets) return error ? <Unread what="presets" /> : <Loading />;

  // A strip's width at a time; the snap lands a key's edge at the start.
  const page = (d) => strip.current.scrollBy({ left: d * strip.current.clientWidth, behavior: 'smooth' });
  // Over the strip's ends, not beside them, so the keys have the card's
  // whole width; the card's colour fades into the keys under the arrow so it
  // reads over any key. Hidden at an end.
  const arrow = (d, on, icon, name) => (
    <button
      type="button"
      aria-label={name}
      aria-hidden={!on}
      tabIndex={on ? 0 : -1}
      onClick={() => page(d)}
      class={`hit absolute inset-y-0 z-10 grid w-8 place-items-center text-ink ${d < 0 ? 'left-0 bg-linear-to-r' : 'right-0 bg-linear-to-l'} from-surface to-transparent ${on ? '' : 'invisible'}`}
    >
      <Icon name={icon} />
    </button>
  );
  return (
    <div class="grid gap-2">
      {!presets.length && <p class="text-sm text-muted">No presets stored yet.</p>}
      <div class="relative min-w-0">
      {arrow(-1, more.left, 'chevron-left', 'Earlier presets')}
        <div
          ref={strip}
          onScroll={look}
          class="relative grid snap-x snap-mandatory auto-cols-[8rem] grid-flow-col gap-2 overflow-x-auto [scrollbar-width:none]"
        >
          {presets.map((p) => (
            <PresetKey p={p} on={p.slot === current} onClick={() => recall(p)} class="snap-start" />
          ))}
          {/* The station on the dial into the next free slot. */}
          <AddKey aria-label="Save this station as a preset" onClick={store} class="snap-start">
            Save this
          </AddKey>
        </div>
      {arrow(1, more.right, 'chevron-right', 'Later presets')}
      </div>
    </div>
  );
}

function LastHeard({ log, error }) {
  if (!logFits.value) return <p class="text-sm text-muted">The station log is not available: its storage did not start.</p>;
  if (!log) return error ? <Unread what="log" /> : <Loading />;
  if (!log.length) return <p class="text-sm text-muted">Nothing logged yet.</p>;
  return log.map((e) => (
    <StationRow e={e} time={e.real && when(e.time)} onClick={() => tune(e.khz)}>
      <span class="truncate text-sm text-muted">
        {dbuv(e.level_dbuv)} dBµV · {e.stereo ? 'stereo' : 'mono'}
      </span>
    </StationRow>
  ));
}

function Facts({ s }) {
  const facts = [
    ['wifi', s.net === 'ap' ? 'Hotspot' : s.rssi == null ? '--' : `${s.rssi} dBm`, 'Wi-Fi'],
    ['power', !s.bat.fit ? 'No battery' : s.bat.boot ? `${(s.bat.boot / 1000).toFixed(2)} V` : 'No reading', 'Battery at start'],
    ['refresh-cw', duration(s.up), 'Up'],
    ['cpu', s.ver, checkSays(s.upd)],
  ];
  return (
    <div class="grid grid-cols-2 gap-4 @lg:grid-cols-4">
      {facts.map(([icon, value, label]) => (
        <div class="flex items-center gap-4">
          <IconTile icon={icon} tone="off" />
          <span>
            <span class="num block text-sm">{value}</span>
            <span class="block text-xs text-muted">{label}</span>
          </span>
        </div>
      ))}
    </div>
  );
}
