// First start: while the radio is on its own hotspot, the page opens here,
// to give it a Wi-Fi network to join. No PIN is needed on the hotspot. It
// can be skipped, to use the radio on its hotspot.
import { signal } from '@preact/signals';
import { useState } from 'preact/hooks';
import { act } from '../data/act.js';
import { WiFiForm } from '../ui/wifi.jsx';
import { Button, Card } from '../ui/kit.jsx';
import { keep, load } from '../data/store.js';

const KEY = 'tef668x-setup-skipped';
// Kept for this tab only, so a later hotspot start asks again.
export const setupSkipped = signal(load(KEY, 'sessionStorage') === '1');

export function FirstStart() {
  const [joining, setJoining] = useState(null);
  function skip() {
    keep(KEY, '1', 'sessionStorage');
    setupSkipped.value = true;
  }
  const send = (fields) => act('/wifi', fields).then((ok) => (ok && setJoining(fields.sid), ok));
  if (joining) {
    return (
      <div class="mx-auto grid w-full max-w-md gap-4">
        <Card title="Saved">
          <p class="text-lg font-semibold">The radio is joining {joining}.</p>
          <ol class="mt-4 grid list-decimal gap-2 pl-4 text-sm">
            <li>Its hotspot stops. Join {joining} on this phone or computer again.</li>
            <li>
              Open <span class="num font-semibold">http://tef668x.local:8080</span>.
            </li>
            <li>If that does not open, the radio's menu has its address: Connectivity, Network Info.</li>
          </ol>
          <p class="mt-4 text-sm text-muted">If it cannot join in 20 s, for example with a wrong passphrase, its hotspot comes back and this page with it.</p>
        </Card>
      </div>
    );
  }
  return (
    <div class="mx-auto grid w-full max-w-md gap-4">
      <Card title="Set up Wi-Fi">
        <p class="mb-4">The radio is on its own hotspot. Give it your Wi-Fi network and it joins it. It needs a 2.4 GHz network.</p>
        <p class="mb-4 text-sm text-muted">This hotspot is open, so the passphrase you type here goes to the radio unencrypted. Do this near the radio, somewhere private.</p>
        <WiFiForm stored={{ sid: '', pss: false }} set={send} />
      </Card>
      <Button kind="quiet" onClick={skip}>
        Use the radio here, on its hotspot
      </Button>
    </div>
  );
}
