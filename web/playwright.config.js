// The page's tests: each runs in a browser against npm run dev's recorded
// radio, on a phone and on a laptop. npm test.
import { defineConfig } from '@playwright/test';

export default defineConfig({
  testDir: 'tests',
  fullyParallel: true,
  // The device's "reduce motion" setting, which turns the page's transitions
  // and animations off, so a test never acts on a view still moving. The
  // tests of the motion itself turn it back on.
  // A failed test keeps its trace in test-results, to replay with
  // npx playwright show-trace.
  use: { baseURL: 'http://localhost:5174', reducedMotion: 'reduce', trace: 'retain-on-failure' },
  webServer: {
    command: 'vite --port 5174 --strictPort',
    url: 'http://localhost:5174',
    // Its own server every run: one left from an earlier run may be on its
    // way down, or serve a module from before an edit.
    reuseExistingServer: false,
  },
  projects: [
    { name: 'phone', use: { viewport: { width: 390, height: 844 }, hasTouch: true } },
    { name: 'laptop', use: { viewport: { width: 1280, height: 800 } } },
  ],
});
