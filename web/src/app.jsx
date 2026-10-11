// The screens, one hash route each, inside the frame.
import { computed, signal } from '@preact/signals';
import { useErrorBoundary } from 'preact/hooks';
import { Frame } from './frame.jsx';
import { Home } from './screens/home.jsx';
import { Radio } from './screens/radio.jsx';
import { Presets } from './screens/presets.jsx';
import { StationLog } from './screens/log.jsx';
import { Dx } from './screens/dx.jsx';
import { BandScope } from './screens/scope.jsx';
import { Settings } from './screens/settings.jsx';
import { Network } from './screens/network.jsx';
import { System } from './screens/system.jsx';
import { FirstStart, setupSkipped } from './screens/setup.jsx';
import { state } from './data/radio.js';
import { Button, Card, Segmented } from './ui/kit.jsx';
import { withMotion } from './data/motion.js';

// Each screen: its route, its name and icon in the menus, and its view.
// tab: one of the four tabs along the foot of a phone.
const SCREENS = [
  { id: '', name: 'Home', icon: 'house', tab: true, view: Home },
  { id: 'radio', name: 'Radio', icon: 'radio', tab: true, view: Radio },
  { id: 'presets', name: 'Presets & Log', icon: 'star', tab: true, view: PresetsAndLog },
  { id: 'dx', name: 'DX', icon: 'satellite-dish', tab: true, view: Dx },
  { id: 'scope', name: 'Band scope', icon: 'chart-column', view: BandScope },
  { id: 'settings', name: 'Settings', icon: 'settings', view: Settings },
  { id: 'network', name: 'Network', icon: 'wifi', view: Network },
  { id: 'system', name: 'System', icon: 'cpu', view: System },
];

const route = signal(location.hash.slice(1));
addEventListener('hashchange', () => withMotion(() => (route.value = location.hash.slice(1))));

// Presets and the station log on one page, as two tabs. #presets and #log
// each open their tab, so a link to either lands on it and the back button
// goes between them.
const TABS = { Presets: 'presets', Log: 'log' };
function PresetsAndLog() {
  const tab = route.value === 'log' ? 'Log' : 'Presets';
  const tabs = <Segmented label="Presets or log" options={Object.keys(TABS)} value={tab} onChange={(t) => (location.hash = TABS[t])} />;
  return tab === 'Log' ? <StationLog tabs={tabs} /> : <Presets tabs={tabs} />;
}

// Setting up Wi-Fi is not one of the screens in the menus: no tab is lit
// for it.
const SETUP = { id: 'setup', name: 'Set up Wi-Fi', view: FirstStart };

// The one thing App reads from the state. It changes only when the radio
// joins or leaves its own hotspot, so a new state, twice a second, does not
// draw the whole page again; each part that shows the state reads it itself.
const onHotspot = computed(() => state.value?.net === 'ap');

export function App() {
  // A fault while a screen is drawn, as from a state this page does not
  // know after the radio took a new firmware: a line and a way out, rather
  // than a blank page. Logged for whoever opens the browser's console.
  const [fault] = useErrorBoundary((e) => console.error(e));
  if (fault) {
    return (
      <main class="mx-auto w-full max-w-5xl p-4">
        <Card>
          <p>This page could not draw what the radio sent.</p>
          <Button icon="refresh-cw" class="mt-4" onClick={() => location.reload()}>
            Reload
          </Button>
        </Card>
      </main>
    );
  }
  const id = route.value === 'log' ? 'presets' : route.value;
  // On its own hotspot the radio has no Wi-Fi yet, or cannot reach it: the
  // page opens on setting it up, until that is skipped. #setup opens it too.
  const setup = route.value === 'setup' || (onHotspot.value && !setupSkipped.value);
  const here = setup ? SETUP : (SCREENS.find((s) => s.id === id) ?? SCREENS[0]);
  return (
    <Frame screens={SCREENS} here={here}>
      <here.view />
    </Frame>
  );
}
