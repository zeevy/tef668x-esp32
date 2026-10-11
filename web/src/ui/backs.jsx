// The faint pictures behind the top card of each screen, each about its
// screen and each moving slowly. A Card draws one given as `back`, behind
// its content and in the accent colour; on a device that asks for less
// motion they stand still.
import { bandOf } from '../data/bands.js';

// The same numbers on every load, so the pictures do not change between
// visits.
function seeded(seed) {
  return () => (seed = (seed * 16807) % 2147483647) / 2147483647;
}

// System: the ESP32 module and the TEF6686 tuner on a board, joined by
// their bus, with the data running along the traces, the board drifting
// very slowly from right to left: drawn twice, side by side, it drifts left
// by its width and starts again, so it never ends.
export function Board() {
  // The module's pads on its right, to the tuner's pins on its left.
  const bus = [0, 1, 2, 3].map((i) => {
    const from = 114.5 + 12 * i;
    const to = 97.5 + 18 * i;
    const bend = 566 + 10 * i;
    return `M534 ${from} H${bend} L${bend + Math.abs(to - from)} ${to} H652`;
  });
  // The module's pads on its left, out to the display's connector.
  const display = [0, 1, 2, 3, 4, 5].map((i) => {
    const y = 90.5 + 12 * i;
    return `M376 ${y} H${330 - 6 * i} L${320 - 6 * i} ${y + 10} H${150 + 12 * i}`;
  });
  // The tuner's antenna in and sound out, to the pads of two connectors.
  const out = [0, 1, 2, 3].map((i) => {
    const y = 88.5 + 18 * i;
    return `M768 ${y} H${800 + 8 * i} L${815 + 8 * i} ${y + (i < 2 ? -15 : 15)} H925`;
  });
  // The small parts' own traces: to each other, to the module's free pads
  // and the tuner's free pins, or down to a via. No trace just stops.
  const ties = [
    'M125 45 H145',
    'M30 50 V100',
    'M120 110 V150 L82 188',
    'M58 194 V224',
    'M207 205 H225 L232 212 H245',
    'M70 194 V224 H186 L193 217 V210',
    'M355 211 H374',
    'M245 45 H260',
    'M282 45 H300 L316 29',
    'M583 54 V70 L562.5 90.5 H536',
    'M607 54 V70 L625.5 88.5 H652',
    'M595 54 V82',
    'M875 30 H782.5 L750.5 62',
    'M975 30 H985',
    'M885 220 H830 L820 210 H812',
    'M985 220 H988',
    'M793 206 V200 L771 178 H750.5',
    'M678.5 178 V190 L663.5 205 H622',
    'M536 186.5 H570 L588.5 205 H600',
  ];
  // Each trace as [path, seconds into its pulse's run, seconds a run].
  const traces = [...bus.map((d, i) => [d, i * 1.8, 6]), ...display.map((d, i) => [d, i * 2.2, 10]), ...out.map((d, i) => [d, i * 3.4, 14]), ...ties.map((d, i) => [d, i * 1.3, 12 + (i % 3) * 2])];
  // Ten pins a side, each a small rectangle sticking out of the chip.
  const pins = Array.from({ length: 10 }, (_, k) => {
    const at = 79.5 + 9 * k;
    const x = 660 + (at - 70);
    return (
      <>
        <rect x="652" y={at - 1.5} width="8" height="3" />
        <rect x="760" y={at - 1.5} width="8" height="3" />
        <rect x={x - 1.5} y="62" width="3" height="8" />
        <rect x={x - 1.5} y="170" width="3" height="8" />
      </>
    );
  });
  // The small parts round the chips, as their outlines on the board: a
  // resistor or a diode lying between two pads, a transistor's flat-sided
  // body over three, a capacitor's round can over two. Each with its name
  // beside it.
  const lying = (x1, x2, y, band) => (
    <>
      <circle cx={x1} cy={y} r="5" />
      <circle cx={x2} cy={y} r="5" />
      <path d={`M${x1 + 5} ${y} H${x1 + 18} M${x2 - 18} ${y} H${x2 - 5}`} />
      <rect x={x1 + 18} y={y - 7} width={x2 - x1 - 36} height="14" rx="3" />
      {band ? <path d={`M${x2 - 26} ${y - 7} V${y + 7}`} stroke-width="4" /> : [0, 1, 2].map((k) => <path d={`M${x1 + 26 + k * 8} ${y - 7} V${y + 7}`} />)}
    </>
  );
  const transistor = (cx, cy) => (
    <>
      <path d={`M${cx - 20} ${cy + 8} A21 21 0 1 1 ${cx + 20} ${cy + 8} Z`} />
      {[-12, 0, 12].map((d) => (
        <circle cx={cx + d} cy={cy} r="3.5" />
      ))}
    </>
  );
  const capacitor = (cx, cy) => (
    <>
      <circle cx={cx} cy={cy} r="17" />
      <circle cx={cx - 7} cy={cy} r="3.5" />
      <circle cx={cx + 7} cy={cy} r="3.5" />
      <path d={`M${cx - 13} ${cy - 22} h6 M${cx - 10} ${cy - 25} v6`} />
    </>
  );
  const parts = [
    [lying(30, 120, 45), 'R1', 75, 30],
    [lying(30, 120, 105, true), 'D1', 75, 90],
    [transistor(70, 190), 'Q1', 104, 186],
    [capacitor(200, 205), 'C1', 230, 196],
    [lying(250, 350, 212), 'R2', 300, 197],
    [lying(150, 240, 45, true), 'D2', 195, 30],
    [transistor(595, 50), 'Q2', 628, 46],
    [lying(880, 970, 30, true), 'D3', 925, 15],
    [lying(890, 980, 220), 'R3', 935, 205],
    [capacitor(800, 210), 'C2', 772, 198],
  ];
  const drawing = () => (
    <>
      <g fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round" opacity="0.2">
        {parts.map(([outline]) => outline)}
        {/* The ESP32 module: its antenna, its can, its pads. */}
        <rect x="380" y="20" width="150" height="210" rx="4" />
        <path d="M392 64 V34 H410 V64 H428 V34 H446 V64 H464 V34 H482 V64 H500 V34 H518 V64" />
        <rect x="392" y="80" width="126" height="138" rx="4" />
        {Array.from({ length: 12 }, (_, j) => (
          <>
            <rect x="374" y={88 + 12 * j} width="10" height="5" />
            <rect x="526" y={88 + 12 * j} width="10" height="5" />
          </>
        ))}
        {/* The tuner: a square chip with ten pins a side, and its crystal. */}
        <rect x="660" y="70" width="100" height="100" rx="3" />
        <circle cx="672" cy="82" r="3" />
        {pins}
        <rect x="682" y="200" width="56" height="22" rx="4" />
        <path d="M696 178 V200 M724 178 V200" />
        {/* The connector to the display, and a few small parts. */}
        {[0, 1, 2, 3, 4, 5].map((i) => (
          <circle cx={146 + 12 * i} cy={100.5 + 12 * i} r="4" />
        ))}
        {[
          [260, 40],
          [600, 200],
          [820, 40],
          [850, 190],
        ].map(([x, y]) => (
          <>
            <rect x={x} y={y} width="8" height="10" rx="1" />
            <rect x={x + 14} y={y} width="8" height="10" rx="1" />
          </>
        ))}
        {traces.map(([d]) => (
          <path d={d} />
        ))}
        <path d="M710 62 V24 M740 62 V36 L752 24" />
        {/* Vias, where a trace goes through to the other side. */}
        {[
          [58, 228],
          [319, 26],
          [595, 86],
          [989, 30],
          [710, 20],
          [755, 21],
          [992, 220],
        ].map(([x, y]) => (
          <>
            <circle cx={x} cy={y} r="4" />
            <circle cx={x} cy={y} r="1.2" />
          </>
        ))}
        {/* The antenna connector and the sound header, two pads each. */}
        <rect x="917" y="62" width="26" height="41" rx="3" />
        <rect x="917" y="128" width="26" height="41" rx="3" />
        {[73.5, 91.5, 139.5, 157.5].map((y) => (
          <circle cx="930" cy={y} r="5" />
        ))}
      </g>
      {/* Short bright pieces running along the traces, one a trace, over
          the faint copper. */}
      <g fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" opacity="0.6">
        {traces.map(([d, delay, slow]) => (
          <path d={d} pathLength="100" class="animate-flow [stroke-dasharray:4_96]" style={{ animationDelay: `-${delay}s`, animationDuration: `${slow}s` }} />
        ))}
      </g>
      <g fill="currentColor" opacity="0.35" font-family="ui-monospace, monospace" font-size="12" text-anchor="middle">
        <text x="455" y="154">
          ESP32
        </text>
        <text x="710" y="124">
          TEF6686
        </text>
        {[[null, 'J1', 930, 57], [null, 'J2', 930, 181], ...parts].map(([, name, x, y]) => (
          <text x={x} y={y} font-size="9">
            {name}
          </text>
        ))}
      </g>
    </>
  );
  return (
    <svg viewBox="0 0 1000 240" preserveAspectRatio="xMidYMid slice" class="absolute inset-0 size-full">
      <g class="animate-drift">
        {drawing()}
        <g transform="translate(1000 0)">{drawing()}</g>
      </g>
    </svg>
  );
}

// A place on the radar for a far station, anywhere inside its rings, and
// its size.
function contact(rnd) {
  const a = rnd() * 2 * Math.PI;
  const d = Math.sqrt(rnd()) * 90;
  return [d * Math.cos(a), d * Math.sin(a), 1.5 + rnd() * 3.5];
}

// DX: a radar scope with its beam turning slowly, a ring going out from
// the middle, and far stations: three at most, each lighting up at a
// random place and size, going out, and coming up somewhere else.
export function Radar() {
  const rnd = seeded(13);
  const blips = [0, 1, 2].map(() => contact(rnd));
  // While it is out, the station moves and takes a new size.
  const elsewhere = (e) => {
    const [x, y, r] = contact(Math.random);
    const g = e.currentTarget;
    g.setAttribute('transform', `translate(${x} ${y})`);
    g.firstChild.setAttribute('r', r);
    g.lastChild.setAttribute('r', r * 2.5);
  };
  return (
    <svg viewBox="-100 -100 200 200" class="absolute top-1/2 left-[80%] h-[200%] -translate-x-1/2 -translate-y-1/2" style={{ aspectRatio: '1' }}>
      <defs>
        <linearGradient id="radar-beam" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2="0" y2="-70">
          <stop offset="0" stop-color="currentColor" stop-opacity="0.35" />
          <stop offset="1" stop-color="currentColor" stop-opacity="0" />
        </linearGradient>
      </defs>
      <g fill="none" stroke="currentColor" stroke-width="0.5" opacity="0.3">
        {[20, 40, 60, 80, 100].map((r) => (
          <circle r={r} />
        ))}
        <path d="M-100 0 H100 M0 -100 V100" />
      </g>
      <circle r="100" fill="none" stroke="currentColor" stroke-width="0.6" class="animate-ripple" style={{ transformOrigin: '0px 0px' }} />
      <g class="animate-sweep" style={{ transformOrigin: '0px 0px' }}>
        <path d="M0 0 L100 0 A100 100 0 0 0 70.7 -70.7 Z" fill="url(#radar-beam)" />
        <path d="M0 0 H100" stroke="currentColor" stroke-width="0.6" opacity="0.6" />
      </g>
      {blips.map(([x, y, r], i) => (
        <g transform={`translate(${x} ${y})`} class="animate-blip" style={{ animationDelay: `-${i * 4}s` }} onAnimationIteration={elsewhere}>
          <circle r={r} fill="currentColor" />
          <circle r={r * 2.5} fill="none" stroke="currentColor" stroke-width="0.5" opacity="0.5" />
        </g>
      ))}
    </svg>
  );
}

// A band's level across the card, 1000 wide and 240 high: a noise floor
// with stations standing out of it, as [x, y] points.
const SCOPE_STATIONS = [
  [90, 0.5],
  [210, 0.9],
  [330, 0.35],
  [450, 1],
  [560, 0.6],
  [690, 0.8],
  [800, 0.45],
  [910, 0.7],
];
function trace(seed) {
  const rnd = seeded(seed);
  return Array.from({ length: 201 }, (_, i) => {
    const x = i * 5;
    let v = 0.06 + 0.06 * rnd();
    for (const [at, h] of SCOPE_STATIONS) v += h * Math.exp(-((x - at) ** 2) / 120);
    return [x, 225 - Math.min(1, v) * 170];
  });
}
const pathOf = (pts) => pts.map(([x, y], i) => `${i ? 'L' : 'M'}${x} ${y.toFixed(1)}`).join(' ');

// Band scope: an analyser's screen, its grid, the band's trace, and the
// stations rising and falling slowly, each at its own pace.
export function Analyzer() {
  return (
    <svg viewBox="0 0 1000 240" preserveAspectRatio="none" class="absolute inset-0 size-full">
      <g stroke="currentColor" stroke-width="1" opacity="0.12">
        {Array.from({ length: 9 }, (_, i) => (
          <path d={`M${(i + 1) * 100} 0 V240`} vector-effect="non-scaling-stroke" />
        ))}
        {Array.from({ length: 5 }, (_, i) => (
          <path d={`M0 ${(i + 1) * 40} H1000`} vector-effect="non-scaling-stroke" />
        ))}
      </g>
      <path d={pathOf(trace(21))} fill="none" stroke="currentColor" stroke-width="1.5" opacity="0.25" vector-effect="non-scaling-stroke" />
      {SCOPE_STATIONS.map(([at, h], i) => (
        <path
          d={`M${at - 40} 225 Q${at} ${225 - h * 340} ${at + 40} 225 Z`}
          fill="currentColor"
          opacity="0.12"
          class="animate-rise-fall"
          style={{ transformOrigin: `${at}px 225px`, animationDelay: `-${i * 3.1}s`, animationDuration: `${14 + (i % 3) * 5}s` }}
        />
      ))}
    </svg>
  );
}

// Network: the bytes going over the air, falling like a waterfall, a
// column of them in hex every so far across, each at its own pace and
// brightness. Each column is drawn twice, one over the other, and falls by
// its height before starting again, so it never ends.
export function Bytes() {
  const rnd = seeded(9);
  const byte = () => Math.floor(rnd() * 256).toString(16).toUpperCase().padStart(2, '0');
  const columns = Array.from({ length: 25 }, (_, i) => [20 + i * 40, Array.from({ length: 12 }, byte), 60 + rnd() * 80, 0.06 + rnd() * 0.1]);
  const column = (x, bytes, top) => (
    <text x={x} text-anchor="middle">
      {bytes.map((b, k) => (
        <tspan x={x} y={top + k * 20 + 14}>
          {b}
        </tspan>
      ))}
    </text>
  );
  return (
    <svg viewBox="0 0 1000 240" preserveAspectRatio="xMidYMid slice" class="absolute inset-0 size-full">
      <g fill="currentColor" font-family="ui-monospace, monospace" font-size="13">
        {columns.map(([x, bytes, slow, bright]) => (
          <g opacity={bright}>
            <g class="animate-rain" style={{ animationDuration: `${slow}s`, animationDelay: `-${(slow * x) % 37}s` }}>
              {column(x, bytes, 0)}
              {column(x, bytes, -240)}
            </g>
          </g>
        ))}
      </g>
    </svg>
  );
}

// The shortwave meter bands, as the firmware's band plan has them: metres,
// low and high edge in kHz.
const METER_BANDS = [
  [160, 1800, 2000],
  [120, 2300, 2495],
  [90, 3200, 3400],
  [75, 3900, 4000],
  [60, 4750, 5060],
  [49, 5900, 6200],
  [41, 7200, 7450],
  [31, 9400, 9900],
  [25, 11600, 12100],
  [22, 13570, 13870],
  [19, 15100, 15800],
  [16, 17480, 17900],
  [15, 18900, 19020],
  [13, 21450, 21850],
  [11, 25670, 26100],
];

// Where a scale starts and ends across the card, in per cent.
const DIAL_FROM = 10;
const DIAL_TO = 96;

// Radio: an old radio's glass dial, a scale for each of FM, AM and SW
// from the radio's own band edges, with the shortwave broadcast bands
// marked, and the tuning needle moving slowly along them while the scales
// move a little the other way.
export function Dial() {
  // Each scale: its name, its height on the card in per cent, its edges in
  // kHz until the radio has said, the kHz a long tick and a number, the
  // kHz a short tick, and how a number is written.
  const scales = [
    ['FM', 24, bandOf('FM') ?? { low: 87500, high: 108000 }, 2000, 500, (k) => k / 1000],
    ['AM', 48, bandOf('MW') ?? { low: 522, high: 1791 }, 200, 50, (k) => k],
    ['SW', 70, bandOf('SW') ?? { low: 1700, high: 27000 }, 2000, 1000, (k) => k / 1000],
  ];
  return (
    <>
      <svg class="absolute inset-0 size-full animate-dial">
        <g fill="currentColor" stroke="currentColor" stroke-width="1" opacity="0.22" font-family="ui-monospace, monospace" font-size="11" text-anchor="middle">
          {scales.map(([name, y, { low, high }, every, tick, words]) => {
            const at = (k) => `${DIAL_FROM + ((k - low) / (high - low)) * (DIAL_TO - DIAL_FROM)}%`;
            const from = Math.ceil(low / tick) * tick;
            const ticks = [];
            for (let k = from; k <= high; k += tick) ticks.push(k);
            return (
              <svg y={`${y}%`} overflow="visible">
                <text x="2%" y="4" stroke="none" text-anchor="start" font-weight="700">
                  {name}
                </text>
                <line x1={`${DIAL_FROM}%`} x2={`${DIAL_TO}%`} />
                {ticks.map((k) => (
                  <line x1={at(k)} x2={at(k)} y1="0" y2={k % every ? -5 : -10} />
                ))}
                {ticks
                  .filter((k) => k % every === 0)
                  .map((k) => (
                    <text x={at(k)} y="-14" stroke="none">
                      {words(k)}
                    </text>
                  ))}
                {name === 'SW' &&
                  METER_BANDS.map(([m, a, b]) => (
                    <>
                      <line x1={at(a)} x2={at(b)} y1="6" y2="6" stroke-width="3" />
                      {m <= 49 && (
                        <text x={at((a + b) / 2)} y="20" stroke="none" font-size="9">
                          {m}
                        </text>
                      )}
                    </>
                  ))}
              </svg>
            );
          })}
        </g>
      </svg>
      <div class="absolute inset-y-[12%] @container" style={{ left: `${DIAL_FROM}%`, right: `${100 - DIAL_TO}%` }}>
        <div class="absolute inset-y-0 left-0 -ml-px w-0.5 animate-needle rounded-full bg-current opacity-45 shadow-[0_0_8px_currentColor]" />
      </div>
    </>
  );
}

// Presets & Log: a night sky. Still stars of every size and brightness,
// and a few that light up and go out, each time somewhere new, at a new
// size and brightness. The colour is the text's, so they are white on the
// dark theme.
export function Stars() {
  const rnd = seeded(5);
  const still = Array.from({ length: 110 }, () => [rnd() * 100, rnd() * 100, 0.3 + rnd() ** 3 * 1.6, 0.1 + rnd() * 0.5]);
  const lit = Array.from({ length: 7 }, (_, i) => [rnd() * 100, rnd() * 100, 8 + rnd() * 14, 0.3 + rnd() * 0.6, i]);
  // Between one lighting and the next, while it is out, the star moves.
  const elsewhere = (e) => {
    const s = e.currentTarget.style;
    const size = 8 + Math.random() * 14;
    s.left = `${Math.random() * 100}%`;
    s.top = `${Math.random() * 100}%`;
    s.width = s.height = `${size}px`;
    s.setProperty('--peak', 0.3 + Math.random() * 0.6);
  };
  return (
    <div class="absolute inset-0 text-ink">
      <svg class="absolute inset-0 size-full">
        {still.map(([x, y, r, o]) => (
          <circle cx={`${x}%`} cy={`${y}%`} r={r} fill="currentColor" opacity={o} />
        ))}
      </svg>
      {lit.map(([x, y, size, peak, i]) => (
        <svg viewBox="-8 -8 16 16" class="absolute -translate-1/2 animate-twinkle" style={{ left: `${x}%`, top: `${y}%`, width: `${size}px`, height: `${size}px`, '--peak': peak, animationDelay: `-${i * 2.6}s` }} onAnimationIteration={elsewhere}>
          <path d="M0 -8 Q0 0 8 0 Q0 0 0 8 Q0 0 -8 0 Q0 0 0 -8 Z" fill="currentColor" />
        </svg>
      ))}
    </div>
  );
}

// A gear of `n` teeth round (cx, cy), outer radius `r`.
function gear(cx, cy, r, n) {
  let d = '';
  for (let i = 0; i < n; i++) {
    const a = (2 * Math.PI * i) / n;
    const w = Math.PI / n / 2;
    for (const [k, rr] of [
      [-w * 1.4, r - 8],
      [-w * 0.7, r],
      [w * 0.7, r],
      [w * 1.4, r - 8],
    ])
      d += `${d ? 'L' : 'M'}${(cx + rr * Math.cos(a + k)).toFixed(1)} ${(cy + rr * Math.sin(a + k)).toFixed(1)} `;
  }
  return `${d}Z`;
}

// Settings: three gears meshing, turning slowly.
export function Gears() {
  const gears = [
    [780, 110, 70, 16, 90, 'normal'],
    [890, 190, 46, 10, 56, 'reverse'],
    [665, 205, 40, 9, 51, 'reverse'],
  ];
  return (
    <svg viewBox="0 0 1000 240" preserveAspectRatio="xMaxYMid slice" class="absolute inset-0 size-full">
      <g fill="none" stroke="currentColor" stroke-width="1.5" stroke-linejoin="round" opacity="0.22">
        {gears.map(([cx, cy, r, n, slow, way]) => (
          <g class="animate-sweep" style={{ transformOrigin: `${cx}px ${cy}px`, animationDuration: `${slow}s`, animationDirection: way }}>
            <path d={gear(cx, cy, r, n)} />
            <circle cx={cx} cy={cy} r={r * 0.3} />
            <circle cx={cx} cy={cy} r={r * 0.55} />
          </g>
        ))}
      </g>
    </svg>
  );
}
