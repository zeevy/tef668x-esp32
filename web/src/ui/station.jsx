// The station on the dial, the same on every screen that shows it: its
// frequency, the chips that say its state, its name with what RDS says of
// it, and the RDS blocks of the last group.
import { useEffect, useRef, useState } from 'preact/hooks';
import { dateLine, dbuv, freq, presetTag, typedKhz, unit } from '../data/format.js';
import { say } from '../data/act.js';
import { tune } from '../data/commands.js';
import { Card, Chip } from './kit.jsx';

// The number's and the unit's sizes.
const SIZES = {
  lg: ['text-[min(4.5rem,20cqw)]', 'text-xl'],
  md: ['text-5xl', 'text-lg'],
};

// The frequency, as Home, Radio, Presets and DX show it: the number, the
// unit with the preset the dial is on over it (P09), and a tap that turns
// the number into a box to type another into, MHz with a point or kHz
// without. Enter tunes there; Escape or a tap away leaves it. The preset
// over the unit is a little smaller than the unit, whatever size that is.
export function Frequency({ t, size = 'lg' }) {
  const [typed, setTyped] = useState(null);
  const box = useRef();
  useEffect(() => box.current?.focus(), [typed === null]);
  const [number, unit] = SIZES[size];
  if (typed === null) {
    return (
      <button type="button" title="Tap to type a frequency" onClick={() => setTyped('')} class="flex items-baseline gap-2 text-left" style={{ viewTransitionName: 'dial' }}>
        <span class={`num leading-none ${number}`}>{t.f}</span>
        <span class={`relative text-muted ${unit}`}>
          {t.unt}
          {t.pst > 0 && <span class="num absolute bottom-full left-0 mb-1 text-[0.7em] leading-none font-semibold text-accent-ink">{presetTag(t.pst)}</span>}
        </span>
      </button>
    );
  }
  function go(e) {
    e.preventDefault();
    const khz = typedKhz(typed);
    setTyped(null);
    if (khz === null) say('Type a frequency, such as 98.3 or 738.', 'bad');
    else tune(khz);
  }
  return (
    <form onSubmit={go} class="flex min-w-0 items-baseline gap-2" style={{ viewTransitionName: 'dial' }}>
      <input
        ref={box}
        aria-label="Frequency to tune"
        inputmode="decimal"
        placeholder={t.f}
        value={typed}
        onInput={(e) => setTyped(e.currentTarget.value)}
        onKeyDown={(e) => e.key === 'Escape' && setTyped(null)}
        onBlur={() => setTyped(null)}
        class={`num w-full min-w-0 bg-transparent leading-none outline-none ${number}`}
      />
      <span class={`text-muted ${unit}`}>{t.unt}</span>
    </form>
  );
}

// What the radio is doing, as chips: the band, stereo, RDS, traffic, mute,
// the squelch, DX mode and a scan; DX and a scan link to their screens.
// `children` go at the end of the row, such as the clock.
export function StationChips({ s, children }) {
  const t = s.tun;
  const r = t.rds ?? {};
  const dxScanning = s.dx?.scn === 'running';
  return (
    <div class="flex flex-wrap items-center gap-2">
      <Chip tone="off">{t.bnd}</Chip>
      {t.st && !t.mno && <Chip>Stereo</Chip>}
      {r.syn && <Chip>RDS</Chip>}
      {r.tp && <Chip>TP</Chip>}
      {r.ta && <Chip tone="warn">TA</Chip>}
      {t.mut && (
        <Chip tone="warn" icon="volume-x">
          Muted
        </Chip>
      )}
      {/* With the squelch on, whether it lets the sound through, and in
          Manual the level it opens at, which only the radio's volume knob
          sets. */}
      {t.sql !== 'Off' && (
        <Chip tone={t.sqo ? 'good' : 'off'}>
          Squelch {t.sqo ? 'open' : 'shut'}
          {t.sql === 'Manual' && t.sqa != null && ` · ${dbuv(t.sqa)} dBµV`}
        </Chip>
      )}
      {s.dx?.on && (
        <a href="#dx">
          <Chip>DX</Chip>
        </a>
      )}
      {(dxScanning || t.scn) && (
        <a href={dxScanning ? '#dx' : '#presets'}>
          <Chip tone="warn">Scanning</Chip>
        </a>
      )}
      {children}
    </div>
  );
}

// The station's name and what RDS says of it. Every line is always there,
// so nothing below moves when a name comes or goes. As on the radio's own
// screen: the station's name, else the preset's, else ---; under it the
// programme type (0 is none set) and the PI, else the date once the radio's
// clock is set; then the radio text.
export function StationText({ s, presets }) {
  const t = s.tun;
  const r = t.rds ?? {};
  return (
    <>
      <p class="mt-2 truncate text-2xl leading-8 font-semibold" style={{ viewTransitionName: 'station-name' }}>
        {r.ps?.trim() || presets?.find((p) => p.slot === t.pst)?.name?.trim() || '---'}
      </p>
      <p class="truncate text-sm leading-5 text-muted">
        {[r.pty > 0 && r.ptn, r.pi && `PI ${r.pi}`].filter(Boolean).join(' · ') || (s.clk.syn ? dateLine(new Date()) : '\u00a0')}
      </p>
      <RadioText r={r} />
    </>
  );
}

// What the station says is on, two lines kept so nothing below moves when it
// comes, goes or changes length. `class` replaces the space above it, for a
// place whose gap already gives that.
export function RadioText({ r, class: cls = 'mt-2' }) {
  return (
    <p class={`line-clamp-2 h-10 text-sm leading-5 ${cls}`} title={r.rt || ''}>
      {r.rt?.trim() || '\u00a0'}
    </p>
  );
}

// The station on the dial in a card's left half, a screen's own tools on its
// right half, as Presets & Log and DX have it, the tabs at the foot of the
// tools. `name`, when given, is drawn in place of the station's name. `presetName` is the
// name of the preset the dial is on, when the station sends none; `sub` a
// line under the name; `chips` in place of the band and preset chips;
// `below` under the line, such as the radio text.
export function StationNow({ s, presetName, name, sub, back, chips, below, children }) {
  const t = s.tun;
  return (
    <Card class="@container" back={back}>
      <div class="grid grid-cols-1 gap-4 @3xl:grid-cols-2">
        <div class="grid content-between gap-2">
          {chips ?? (
            <div class="flex gap-2">
              <Chip tone="off">{t.bnd}</Chip>
              {!(t.pst > 0) && <Chip tone="off">Not a preset</Chip>}
            </div>
          )}
          <Frequency t={t} size="md" />
          <p class="truncate text-xl leading-7 font-semibold" style={{ viewTransitionName: 'station-name' }}>
            {name ?? (t.rds?.ps?.trim() || presetName || '---')}
          </p>
          <p class="text-sm text-muted">{sub || '\u00a0'}</p>
          {below}
        </div>
        <div class="grid content-between gap-2">{children}</div>
      </div>
    </Card>
  );
}

// A logged or caught station's name: the one it sent or was given, else its
// PI, else ---.
export const stationLabel = (e) => e.name?.trim() || (e.pi ? `PI ${e.pi}` : '---');

// A logged or caught station as a row, as Last heard, the log and DX's
// catches list them: the frequency, its unit and name on the first line,
// then `tag` and, at the right, `time`; under it `children`. The row is a
// button for `onClick`; `after` sits at its end, outside it, such as a
// button of its own.
export function StationRow({ e, tag, time, onClick, after, children }) {
  return (
    <div class="flex min-h-14 items-center gap-2 border-b border-line py-2 last:border-0">
      <button type="button" onClick={onClick} class="grid min-w-0 flex-1 gap-1 text-left">
        <span class="flex items-baseline gap-2">
          <span class="num font-semibold">{freq(e.khz, e.band)}</span>
          <span class="text-sm text-muted">{unit(e.band)}</span>
          <span class="min-w-0 truncate font-semibold">{stationLabel(e)}</span>
          {tag}
          {time && <span class="ml-auto shrink-0 text-sm text-muted">{time}</span>}
        </span>
        {children}
      </button>
      {after}
    </div>
  );
}

// The station's alternative frequencies from RDS, a tap tunes one; nothing
// while it sends none.
export function AfList({ t }) {
  const af = t.rds?.af;
  if (!af?.length) return null;
  return (
    <div class="grid gap-1">
      <span class="label">Alternative frequencies</span>
      <div class="flex flex-wrap gap-2">
        {af.map((khz) => (
          <button
            type="button"
            aria-pressed={khz === t.khz}
            title={`Tune to ${freq(khz, t.bnd)} ${t.unt}`}
            onClick={() => tune(khz)}
            class={`num h-9 rounded-ctl px-2 text-sm ${khz === t.khz ? 'bg-accent/15 text-accent-ink' : 'bg-raised'}`}
          >
            {freq(khz, t.bnd)}
          </button>
        ))}
      </div>
    </div>
  );
}

const NAMES = ['A', 'B', 'C', 'D'];

// Blocks A to D of the last group as the radio's DX page draws them: four
// segments lit for a clean block, three or two for an error the tuner fixed,
// one for an error it could not fix, none with no RDS.
export function BlockTile({ r }) {
  const lit = (i) => (r.syn && r.blk ? 4 - Math.min(3, r.blk[i]) : 0);
  return (
    <div
      class="grid content-start gap-1 rounded-ctl bg-raised p-2"
      role="img"
      aria-label={`RDS blocks: ${NAMES.map((name, i) => `${name} ${lit(i)} of 4`).join(', ')}`}
    >
      <span class="label">Blocks</span>
      <div class="flex gap-1">
        {NAMES.map((name, i) => (
          <div class="grid flex-1 justify-items-center gap-1">
            <div class="flex w-full flex-col-reverse gap-1">
              {[0, 1, 2, 3].map((k) => (
                <span class={`h-1.5 rounded-sm ${k < lit(i) ? 'bg-good' : 'bg-surface'}`} />
              ))}
            </div>
            <span class="text-xs leading-4 font-semibold">{name}</span>
          </div>
        ))}
      </div>
    </div>
  );
}
