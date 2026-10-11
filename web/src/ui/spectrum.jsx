// The band behind the radio's reading on Home.
import { levelY } from './sweep.jsx';

// Behind the reading on Home, faint, the band as the band scope last swept
// it, with the dial's place marked and the station there drawn from the
// level now. A sweep mutes the radio while it reads the band, so this never
// asks for one. With no whole sweep of this band yet, the station alone: a
// peak in the middle as tall as the signal on a floor as high as the noise,
// with a fixed ripple so the floor is not a ruled line.
const SPECTRUM_W = 1000;
const SPECTRUM_H = 300;
// Drawn at a fixed scale, 8 px a channel as the scale on Radio, and never
// under SPECTRUM_MIN_PX, so it is wider than the card on any screen and never
// squeezed; it pans across the card instead, as wide as the box it is in.
// Drawn as the card's `back`, so with the motion turned down on the device it
// stays still, at the band's start.
const PX_PER_CHANNEL = 8;
const SPECTRUM_MIN_PX = 1600;
export function Spectrum({ t, sweep }) {
  // A whole sweep of this band that holds the dial: on SW a sweep of one
  // metre band says nothing once the dial is in another.
  const swept = sweep?.whole && sweep.band === t.bnd && t.khz >= sweep.low && t.khz <= sweep.low + sweep.step * (sweep.count - 1);
  const width = Math.max(SPECTRUM_MIN_PX, swept ? sweep.count * PX_PER_CHANNEL : 0);
  // Heights from 0 at the foot to 1 at the top, against x from 0 to 1.
  const height = (v) => 1 - levelY(sweep ?? {}, v) / 100;
  let dial = 0.5;
  let at;
  if (swept) {
    dial = (t.khz - sweep.low) / (sweep.step * (sweep.count - 1));
    const live = height(t.sig);
    at = sweep.level.map((v, i) => {
      const x = i / (sweep.count - 1);
      return [x, Math.max(height(v), live * Math.exp(-((x - dial) ** 2) / 0.0004))];
    });
  } else {
    const peak = Math.max(0, Math.min(1, t.sig / 700));
    const floor = 0.08 + Math.min(0.2, t.usn / 2000);
    at = [];
    for (let i = 0; i <= 200; i++) {
      const x = i / 200;
      at.push([x, floor + 0.03 * Math.sin(x * 97) * Math.sin(x * 31) + peak * Math.exp(-((x - 0.5) ** 2) / 0.004)]);
    }
  }
  // A bar a channel, three quarters of its share wide, fading upwards.
  const share = SPECTRUM_W / at.length;
  return (
    <div class="absolute inset-0 @container">
    <svg viewBox={`0 0 ${SPECTRUM_W} ${SPECTRUM_H}`} preserveAspectRatio="none" class="absolute inset-y-0 left-0 h-full animate-pan" style={{ width: `${width}px` }}>
      <defs>
        <linearGradient id="spectrum-fade" x1="0" y1="1" x2="0" y2="0">
          <stop offset="0" stop-color="currentColor" stop-opacity="0.3" />
          <stop offset="1" stop-color="currentColor" stop-opacity="0.08" />
        </linearGradient>
      </defs>
      {at.map(([x, h]) => {
        const tall = Math.max(0, h) * SPECTRUM_H * 0.85;
        return <rect x={x * (SPECTRUM_W - share) + share * 0.125} y={SPECTRUM_H - tall} width={share * 0.75} height={tall} fill="url(#spectrum-fade)" />;
      })}
      {swept && <line x1={dial * (SPECTRUM_W - share) + share / 2} x2={dial * (SPECTRUM_W - share) + share / 2} y1="0" y2={SPECTRUM_H} stroke="currentColor" stroke-width="1" opacity="0.2" vector-effect="non-scaling-stroke" />}
    </svg>
    </div>
  );
}
