// Radio: the reading with the band scale and the radio's controls, then RDS
// and the signal.
import { useEffect, useRef } from 'preact/hooks';
import { state } from '../data/radio.js';
import { presets as presetsRead } from '../data/reads.js';
import { bandOf } from '../data/bands.js';
import { BANDS, country, freq, isFm, readings, unit } from '../data/format.js';
import { act } from '../data/act.js';
import { recall, seek, step, tune } from '../data/commands.js';
import { Bar, Button, Card, Chip, FactRows, Figure, Meter, Segmented, Select } from '../ui/kit.jsx';
import { KeysHelp, useKeys } from '../ui/keys.jsx';
import { AfList, BlockTile, Frequency, StationChips, StationText } from '../ui/station.jsx';
import { Dial } from '../ui/backs.jsx';

const MODES = [
  ['Manual', 'Manual'],
  ['Auto', 'Auto'],
  ['Presets', 'Presets'],
  ['Meter band', 'MeterBand'],
];
const SQUELCH = [
  ['Off', 'off'],
  ['Auto', 'auto'],
  ['Manual', 'manual'],
];

export function Radio() {
  const s = state.value;
  // Null while not known, so a failed read leaves no marks on the scale.
  const presets = presetsRead.use().data;
  // The same keys as on Home.
  const typedSlot = useKeys(presets);
  // The radio across the page, the reading on the left and its controls on
  // the right; under it RDS on the left and the signal on the right, the two
  // the same height. One column on a phone, with RDS
  // last.
  return (
    <div class="grid grid-cols-1 gap-4">
      <Card class="@container" back={<Dial />}>
        <div class="grid grid-cols-1 gap-4 @3xl:grid-cols-2">
          <Reading s={s} presets={presets} />
          <Controls s={s} />
        </div>
      </Card>
      <div class="grid grid-cols-1 gap-4 lg:grid-cols-2">
        <Signal s={s} />
        <Rds s={s} />
      </div>
      <KeysHelp typedSlot={typedSlot} />
    </div>
  );
}

function Reading({ s, presets }) {
  const t = s.tun;
  return (
    <div class="flex flex-col">
      <StationChips s={s} />
      <div class="mt-4">
        <Frequency t={t} />
      </div>
      <StationText s={s} presets={presets} />
      {/* At the foot, level with the last row of controls beside it. */}
      <div class="mt-auto pt-4">
        <Scale s={s} presets={presets} />
      </div>
    </div>
  );
}

// The band as a sliding scale, as the radio's own screen draws it: a mark
// every 100 kHz on FM and every 10 kHz on AM, TICK_PX apart, long and
// numbered every tenth, medium every fifth. A swipe scrolls it, and it
// stops at the band's ends; each tune brings the dial back to the middle, or
// as near as the ends allow. A tap tunes there, to the nearest channel on
// the dial's own grid, counted from the dial: OIRT's 30 kHz and FM's 200 kHz
// steps do not start at 0 kHz. It is a slider for the keys: once it has
// focus, Left and Right step the dial, as this screen's keys do anywhere on
// it. Over the marks a dot for each preset on this band, a button: its name
// shows on hover and focus, and a tap recalls it.
const TICK_PX = 8;
// Room at each end for the end number: SW's 27000 is 32 px wide.
const END_PX = 16;

function Scale({ s, presets }) {
  const t = s.tun;
  // The band's own ends, as the radio's band plan has them.
  const band = bandOf(t.bnd);
  const tick = isFm(t.bnd) ? 100 : 10;
  const x = (k) => END_PX + ((k - band.low) * TICK_PX) / tick;
  const box = useRef();
  const shown = useRef(false);
  useEffect(() => {
    if (!band || !box.current) return;
    const row = box.current;
    row.scrollTo({ left: x(t.khz) - row.clientWidth / 2, behavior: shown.current ? 'smooth' : 'auto' });
    shown.current = true;
  }, [t.khz, t.bnd, band?.low]);
  if (!band) return <div class="h-14" aria-hidden="true" />;
  const first = (every) => Math.ceil(band.low / every) * every;
  // Each kind of mark is one background repeated along the row, so SW's
  // 2,500 marks are three backgrounds and not 2,500 elements.
  const layers = [
    [tick, 6],
    [tick * 5, 10],
    [tick * 10, 16],
  ];
  const style = {
    width: `${x(band.high) + 1 + END_PX}px`,
    backgroundImage: layers.map(() => 'linear-gradient(to right, var(--color-muted) 1px, transparent 1px)').join(','),
    backgroundSize: layers.map(([every, h]) => `${(every * TICK_PX) / tick}px ${h}px`).join(','),
    backgroundPosition: layers.map(([every]) => `${x(first(every))}px calc(100% - 16px)`).join(','),
    backgroundRepeat: 'repeat-x',
  };
  const numbers = [];
  for (let k = first(tick * 10); k <= band.high; k += tick * 10) numbers.push(k);
  const marks = (presets ?? []).filter((p) => p.band === t.bnd && p.khz >= band.low && p.khz <= band.high);
  function tap(e) {
    const left = e.currentTarget.getBoundingClientRect().left;
    const raw = band.low + ((e.clientX - left - END_PX) * tick) / TICK_PX;
    tune(Math.min(band.high, Math.max(band.low, t.khz + Math.round((raw - t.khz) / t.stp) * t.stp)));
  }
  return (
    <div ref={box} class="overflow-x-auto overflow-y-hidden overscroll-x-contain select-none [scrollbar-width:none]">
        <div class="relative h-14" style={{ width: style.width }}>
          {/* The focus ring is drawn inside, as the scroll box clips its foot. */}
          <div
            class="absolute inset-x-0 bottom-0 h-9 cursor-pointer focus-visible:outline-2 focus-visible:-outline-offset-2 focus-visible:outline-accent"
            style={{ ...style, width: undefined }}
            onClick={tap}
            tabIndex={0}
            role="slider"
            aria-label="Band scale"
            aria-valuemin={band.low}
            aria-valuemax={band.high}
            aria-valuenow={t.khz}
            aria-valuetext={`${t.f} ${t.unt}`}
          >
            <div class="absolute inset-x-0 bottom-4 h-px bg-line" />
            {numbers.map((k) => (
              <span class="num absolute bottom-0 -translate-x-1/2 text-xs leading-4 text-muted" style={{ left: `${x(k)}px` }}>
                {isFm(t.bnd) ? k / 1000 : k}
              </span>
            ))}
            <div data-mark="dial" class="absolute bottom-4 h-5 w-1 -translate-x-1/2 rounded-ctl bg-accent" style={{ left: `${x(t.khz)}px` }} />
          </div>
          {marks.map((p) => {
            const name = `Preset ${p.slot}, ${p.name?.trim() || freq(p.khz, p.band)}`;
            return (
              <button
                type="button"
                aria-label={`${name}, ${freq(p.khz, p.band)} ${unit(p.band)}`}
                aria-pressed={p.slot === t.pst}
                onClick={() => recall(p)}
                class="group hit absolute top-0 flex size-5 -translate-x-1/2 items-center justify-center"
                style={{ left: `${x(p.khz)}px` }}
              >
                <span class={`size-2 rounded-full ${p.slot === t.pst ? 'bg-accent' : 'bg-muted'}`} />
                <span class="pointer-events-none absolute top-full left-1/2 z-10 hidden -translate-x-1/2 rounded-ctl bg-ink px-2 py-1 text-xs whitespace-nowrap text-surface group-hover:block group-focus-visible:block">
                  {name}
                </span>
              </button>
            );
          })}
        </div>
    </div>
  );
}

// The radio's controls, beside the reading, one row each and no labels: the
// bands and the icons name themselves, and the drop-downs say what they are
// in their own text. Step and filter, then seek and step, at the foot. The
// rows spread to the reading's height.
function Controls({ s }) {
  const t = s.tun;
  // The steps and filter widths this band offers; 0 is the tuner's own
  // choice of width.
  const band = bandOf(t.bnd);
  const flip = (key) => (on) => act('/api/fm', { [key]: +on }, (n) => (n.tun[key] = on));
  return (
    <div class="grid content-between gap-4 @3xl:gap-2">
      <Segmented label="Band" options={BANDS} value={t.bnd} onChange={(bnd) => act('/api/band', { bnd })} />
      <div class="grid grid-cols-1 gap-2 @xs:grid-cols-2">
        <Select
          label="Tuning mode"
          value={t.tmd}
          onChange={(name) => act('/api/mode', { mod: MODES.find(([n]) => n === name)[1] }, (n) => (n.tun.tmd = name))}
        >
          {MODES.map(([name]) => (
            <option value={name}>Tuning {name.toLowerCase()}</option>
          ))}
        </Select>
        <Select
          label="Squelch"
          value={t.sql}
          onChange={(name) => act('/api/squelch', { mod: SQUELCH.find(([n]) => n === name)[1] }, (n) => (n.tun.sql = name))}
        >
          {SQUELCH.map(([name]) => (
            <option value={name}>Squelch {name.toLowerCase()}</option>
          ))}
        </Select>
      </div>
      {/* The tuner's FM sound switches, as the radio's FM Reception menu has
          them. */}
      {isFm(t.bnd) && (
        <div class="grid grid-cols-3 gap-2">
          <Lamp label="Mono" on={!!t.mno} onChange={flip('mno')} />
          <Lamp label="iMS" on={!!t.ims} onChange={flip('ims')} />
          <Lamp label="EQ" on={!!t.eq} onChange={flip('eq')} />
        </div>
      )}
      <div class="grid grid-cols-1 gap-2 @xs:grid-cols-2">
        <Select label="Step" value={t.stp} onChange={(v) => act('/api/step-size', { khz: +v }, (n) => (n.tun.stp = +v))}>
          {(band?.steps ?? []).map((k) => (
            <option value={k}>Step {k} kHz</option>
          ))}
        </Select>
        <Select label="Bandwidth" value={t.bws} onChange={(v) => act('/api/bandwidth', { khz: +v }, (n) => (n.tun.bws = +v))}>
          {(band?.widths ?? []).map((k) => (
            <option value={k}>{k === 0 ? 'Filter auto' : `Filter ${k} kHz`}</option>
          ))}
        </Select>
      </div>
      <div class="grid grid-cols-4 gap-2">
        {[
          ['skip-back', 'Seek down', () => seek(false)],
          ['chevron-left', 'Step down', () => step(-1)],
          ['chevron-right', 'Step up', () => step(1)],
          ['skip-forward', 'Seek up', () => seek(true)],
        ].map(([icon, label, send]) => (
          <Button kind="secondary" icon={icon} aria-label={label} title={label} onClick={send} />
        ))}
      </div>
    </div>
  );
}

// An on or off key with a lamp, lit when on.
function Lamp({ label, on, onChange }) {
  return (
    <button
      type="button"
      role="switch"
      aria-checked={on}
      onClick={() => onChange(!on)}
      class={`flex h-11 items-center justify-center gap-2 rounded-ctl px-4 font-medium ${on ? 'bg-accent-soft text-accent-ink' : 'bg-raised text-muted'}`}
    >
      <span class={`size-2 rounded-full ${on ? 'bg-accent' : 'bg-muted/50'}`} />
      {label}
    </button>
  );
}

// The signal: the level and its noise as bars, then three figures. Spread
// over the height RDS beside it takes.
function Signal({ s }) {
  const t = s.tun;
  const r = readings(t.bnd, t);
  return (
    <Card title="Signal" end={t.plt ? <Chip tone="good">Pilot</Chip> : null} class="flex flex-col lg:col-start-2 lg:row-start-1">
      <div class="grid flex-1 content-between gap-4">
        <Meter {...r.level} />
        <Meter {...r.snr} label="Signal to noise" />
        {r.noise && <Meter {...r.noise} />}
        <Meter {...r.wam} />
        <div class="grid grid-cols-3 gap-4">
          {[r.offset, r.width, r.mod].map((f) => (
            <Figure {...f} />
          ))}
        </div>
      </div>
    </Card>
  );
}


// How well RDS is heard on this station: of the blocks the tuner handed over,
// four a group, the shares that came clean, that it fixed and that it lost;
// and the share of groups something was decoded from. Null before the first
// group.
function rdsQuality(r) {
  const blocks = (r.grp ?? 0) * 4;
  if (!blocks) return null;
  const fixed = (r.cor ?? 0) / blocks;
  const lost = (r.bad ?? 0) / blocks;
  return { clean: Math.max(0, 1 - fixed - lost), fixed, lost, used: (r.use ?? 0) / r.grp };
}

const pc = (v) => Math.round(v * 100);

// What RDS says of the station, as rows the same as the controls': the
// blocks of the last group and the quality on top, then the facts, label
// left and value right, then the other frequencies, which tune. A fact the
// station has not sent is ---, so nothing moves when it comes.
function Rds({ s }) {
  const t = s.tun;
  const r = t.rds ?? {};
  const q = rdsQuality(r);
  const rows = [
    ['PI', r.pi, true],
    ['Programme type', r.pty > 0 ? r.ptn : null],
    ['Traffic', r.ta ? 'Announcement on' : r.tp ? 'Traffic station' : r.syn ? 'No traffic' : null],
    ['Music or speech', r.ms ? r.ms[0].toUpperCase() + r.ms.slice(1) : null],
    ['Country', r.cty ? country(r.cty) : null],
    ['Language', r.lgn ?? (r.lng ? `Code ${r.lng}` : null)],
    // The station's own local time, as it sent it, not moved to this
    // browser's time zone.
    ['Station clock', r.ct ? `${r.ct.slice(11, 16)}, ${new Date(r.ct.slice(0, 10)).toLocaleDateString('en-GB', { day: 'numeric', month: 'short', timeZone: 'UTC' })}` : null],
    ['Groups seen', r.grp ?? 0, true],
  ];
  return (
    <Card title="RDS" class="@container flex flex-col lg:col-start-1 lg:row-start-1">
      {/* The rows take what height the card has over, when the signal
          beside it is taller, so no space is left empty at the foot. */}
      <div class={`grid flex-1 gap-4 ${r.af?.length ? 'grid-rows-[auto_1fr_auto]' : 'grid-rows-[auto_1fr]'}`}>
        <div class="grid grid-cols-[7rem_1fr] gap-4">
          <BlockTile r={r} />
          <div class="grid content-between gap-2">
            <Bar label={q ? `${pc(q.clean)} % clean, ${pc(q.fixed)} % fixed, ${pc(q.lost)} % lost` : 'No RDS yet'}>
              {q && (
                <>
                  <div class="bg-good" style={{ width: `${q.clean * 100}%` }} />
                  <div class="bg-warn" style={{ width: `${q.fixed * 100}%` }} />
                  <div class="bg-bad" style={{ width: `${q.lost * 100}%` }} />
                </>
              )}
            </Bar>
            <div class="grid grid-cols-2 gap-x-4 gap-y-2 @sm:grid-cols-4">
              {[
                ['Clean', q?.clean, 'text-good'],
                ['Fixed', q?.fixed, 'text-warn'],
                ['Lost', q?.lost, 'text-bad'],
                ['Used', q?.used, ''],
              ].map(([label, v, tone]) => (
                <Figure label={label} value={v == null ? '--' : pc(v)} unit="%" tone={tone} />
              ))}
            </div>
          </div>
        </div>
        <FactRows rows={rows} class="grid auto-rows-fr gap-x-4 @md:grid-cols-2" />
        <AfList t={t} />
      </div>
    </Card>
  );
}
