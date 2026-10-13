// Network: how the radio is connected, the Wi-Fi it joins, its hotspot, the
// PC Link, the access PIN, and switching Wi-Fi or the web server off. The
// connection large on top, then a card of rows.
import { useState } from 'preact/hooks';
import { pinAsked, signedIn, state } from '../data/radio.js';
import { useStoredSettings } from '../data/reads.js';
import { say } from '../data/act.js';
import { Ask, Banner, Button, Card, Chip, INPUT, Loading, Modal, PinNeeded, Segmented, SettingRow, StatusHead, Switch, Unread } from '../ui/kit.jsx';
import { Bytes } from '../ui/backs.jsx';
import { WiFiForm } from '../ui/wifi.jsx';

const HOTSPOT = ['Auto', 'On', 'Off'];

export function Network() {
  const s = state.value;
  const { stored, set, error, needPin } = useStoredSettings();
  if (needPin) return <PinNeeded what="network settings" />;
  if (!stored) return <Card>{error ? <Unread what="network settings" /> : <Loading />}</Card>;
  return <StatusAndRows s={s} stored={stored} set={set} />;
}

// The connection, then every item as a row as in Settings, the two turn
// offs too; Wi-Fi and the PIN open their form in a dialog. The hotspot and
// the PC Link are drawn at once and put back on a refusal; the turn offs are
// each asked first.
function StatusAndRows({ s, stored, set }) {
  const [open, setOpen] = useState(null);
  const [asking, setAsking] = useState(null); // { title, text, no, yes, kind, go }
  const ips = s.pcl?.ips ?? [];
  const close = () => setOpen(null);
  const home = stored.sid || 'your network';
  const applyHotspot = (word) => set({ hsp: HOTSPOT.indexOf(word) });
  // A choice that takes the radio off the network this page reaches it on is
  // asked first: On leaves your network for the hotspot alone, and Auto or
  // Off, picked over the hotspot, leaves the hotspot.
  const hotspot = (word) => {
    const onHotspot = s.net === 'ap';
    if (word === 'On' && !onHotspot) {
      setAsking({
        title: 'Turn the hotspot on?',
        text: `The radio leaves ${home} and serves only its own hotspot, tef668x-setup- and four characters. This page loses the radio: join that hotspot and open http://192.168.4.1:8080 to reach it again.`,
        no: 'Stay on Wi-Fi',
        yes: 'Turn it on',
        go: () => applyHotspot(word),
      });
    } else if (word !== 'On' && onHotspot) {
      setAsking({
        title: `Set the hotspot to ${word}?`,
        text: stored.sid
          ? `The radio leaves its hotspot and joins ${stored.sid}${word === 'Off' ? ', and does not come back to the hotspot if it cannot' : ''}. This page loses the radio here: find it on ${stored.sid} at tef668x.local:8080.`
          : `No network is stored, so the radio ${word === 'Off' ? 'will be on no network at all; only its menu brings the hotspot back' : 'stays on its hotspot'}.`,
        no: 'Keep the hotspot',
        yes: `Set it to ${word}`,
        go: () => applyHotspot(word),
      });
    } else applyHotspot(word);
  };
  const pcl = (on) => set({ pcl: +on });
  // Wi-Fi or the web server off; only the radio's menu turns them on again.
  const what = { wif: ['Wi-Fi', 'The radio leaves the network'], web: ['the web server', 'This page and the HTTP API stop'] };
  const turnOff = (key) =>
    setAsking({
      title: `Turn off ${what[key][0]}?`,
      text: `${what[key][1]}. Only the radio's menu, Connectivity, turns it on again.`,
      no: 'Keep it on',
      yes: 'Turn off',
      kind: 'danger',
      go: async () => {
        if (await set({ [key]: 0 })) say(`${what[key][0][0].toUpperCase()}${what[key][0].slice(1)} is off.`);
      },
    });
  return (
    <div class="grid grid-cols-1 gap-4">
      <Hero s={s} stored={stored} />
      <Card>
        <SettingRow title="Wi-Fi network" note={stored.sid || 'None stored'}>
          <Button kind="secondary" class="w-full" onClick={() => setOpen('wifi')}>
            Change
          </Button>
        </SettingRow>
        <SettingRow title="Hotspot" note="Auto: only when the Wi-Fi cannot be joined">
          <Segmented label="Hotspot" options={HOTSPOT} value={HOTSPOT[stored.hsp] ?? 'Auto'} onChange={hotspot} />
        </SettingRow>
        <SettingRow title="PC Link" note={stored.pcl ? (ips.length ? `Connected: ${ips.join(', ')}` : 'Port 7373, no PC connected') : 'Port 7373, for XDR-GTK and FM-DX Webserver'}>
          <Switch label="PC Link" text={stored.pcl ? 'On' : 'Off'} on={!!stored.pcl} onChange={pcl} />
        </SettingRow>
        <SettingRow title="Access PIN" note={s.dpn ? 'Still the default PIN: anyone on your network can change the radio' : 'Changed from the default'} warn={s.dpn}>
          <Button kind="secondary" class="w-full" onClick={() => setOpen('pin')}>
            Change
          </Button>
        </SettingRow>
        <SettingRow title="Wi-Fi" note="Turn off; only the radio's menu turns it on again">
          <Button kind="danger" icon="power" class="w-full" onClick={() => turnOff('wif')}>
            Turn off
          </Button>
        </SettingRow>
        <SettingRow title="Web server" note="Turn off; this page and the HTTP API stop">
          <Button kind="danger" icon="power" class="w-full" onClick={() => turnOff('web')}>
            Turn off
          </Button>
        </SettingRow>
      </Card>
      {open && (
        <Modal title={open === 'wifi' ? 'Wi-Fi network' : 'Access PIN'} size="lg" onClose={close}>
          {open === 'wifi' ? <WiFiForm stored={stored} set={set} done={close} /> : <PinForm s={s} set={set} done={close} />}
        </Modal>
      )}
      {asking && <Ask title={asking.title} text={asking.text} no={asking.no} yes={asking.yes} kind={asking.kind} onYes={asking.go} onClose={() => setAsking(null)} />}
    </div>
  );
}

// The connection, large: the network, the signal as bars, the address, and
// the hotspot, PC Link and PIN at a glance.
// The Wi-Fi signal as the Wi-Fi symbol drawn with five curved bars: the dot,
// lit while connected, then four arcs, one each above -80, -70, -60 and
// -50 dBm, 10 dB steps that hold the radio's own panel's three. The bars not
// reached stay faint.
const ARC_DBM = [-80, -70, -60, -50];
const arc = (r) => {
  const d = r * Math.SQRT1_2;
  return `M${12 - d} ${21 - d}A${r} ${r} 0 0 1 ${12 + d} ${21 - d}`;
};

function WifiSignal({ rssi }) {
  const lit = ARC_DBM.filter((dbm) => rssi > dbm).length;
  return (
    <svg viewBox="0 0 24 24" width="32" height="32" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" role="img" aria-label={`${lit + 1} of 5 bars, ${rssi} dBm`}>
      <path d="M12 21h.01" />
      {ARC_DBM.map((_, i) => (
        <path d={arc(4 + i * 4)} opacity={i < lit ? 1 : 0.3} />
      ))}
    </svg>
  );
}

function Hero({ s, stored }) {
  const words = {
    station: `On ${stored.sid || 'the Wi-Fi'}`,
    ap: s.apn ? `On its own hotspot, ${s.apn}` : 'On its own hotspot',
    joining: `Joining ${stored.sid || 'the Wi-Fi'}`,
    offline: 'Off the network',
  };
  return (
    // On a network, the Wi-Fi icon lit by the signal; on the hotspot or off
    // it, the plain icon.
    <Card title="Connection" back={<Bytes />}>
      <StatusHead icon={s.net === 'station' && s.rssi != null ? <WifiSignal rssi={s.rssi} /> : 'wifi'} title={words[s.net] ?? s.net} sub={[s.net === 'station' && s.rssi != null && `${s.rssi} dBm`, s.ip && `address ${s.ip}`, 'port 8080'].filter(Boolean).join(' · ')} />
      <div class="mt-4 flex flex-wrap gap-2">
        <Chip tone="off">Hotspot {HOTSPOT[stored.hsp] ?? 'Auto'}</Chip>
        <Chip tone={stored.pcl ? 'accent' : 'off'}>PC Link {stored.pcl ? `on, ${s.pcl?.cli ?? 0} PC` : 'off'}</Chip>
        {s.dpn && <Chip tone="warn">Default PIN</Chip>}
      </div>
    </Card>
  );
}

// A new access PIN, typed twice. It signs every browser and PC out.
function PinForm({ s, set, done }) {
  const [pin, setPin] = useState('');
  const [again, setAgain] = useState('');
  const digits = (v) => v.replace(/\D/g, '').slice(0, 6);
  async function change(e) {
    e.preventDefault();
    if (pin.length !== 6) return say('A PIN is six digits.', 'bad');
    if (pin !== again) return say('The two PINs are not the same.', 'bad');
    // Sent, never drawn: the page keeps no copy of the PIN.
    if (await set({ pin }, '/api/settings', {})) {
      done?.();
      say('PIN changed. Sign in with the new one.');
      signedIn.value = false;
      pinAsked.value = true;
    }
  }
  return (
    <>
      {s.dpn && <Banner>The radio still uses the default PIN. Anyone on your network can change the radio.</Banner>}
      <form onSubmit={change} class="grid gap-4">
        <div class="grid grid-cols-2 gap-2">
          <label class="grid gap-1">
            <span class="label">New PIN</span>
            <input class={`${INPUT} num`} inputmode="numeric" type="password" value={pin} onInput={(e) => setPin(digits(e.currentTarget.value))} autocomplete="new-password" />
          </label>
          <label class="grid gap-1">
            <span class="label">Again</span>
            <input class={`${INPUT} num`} inputmode="numeric" type="password" value={again} onInput={(e) => setAgain(digits(e.currentTarget.value))} autocomplete="new-password" />
          </label>
        </div>
        <Button type="submit" icon="lock">
          Change PIN
        </Button>
        <p class="text-sm text-muted">It signs every browser and every PC out.</p>
      </form>
    </>
  );
}

