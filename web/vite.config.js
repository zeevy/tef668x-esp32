// The radio's web interface: one page, built into one index.html.
//
//   npm run dev        the page with data recorded from the radio, no radio
//                      needed: GET routes answer from mock/, POSTs answer ok
//   npm run dev:radio  the page on the live radio: every call is forwarded to
//                      it, the PIN session through it, nothing flashed
//   npm run build      dist/index.html, everything inlined, and its gzipped size
import fs from 'node:fs';
import path from 'node:path';
import { defineConfig, loadEnv } from 'vite';
import tailwindcss from '@tailwindcss/vite';
import { viteSingleFile } from 'vite-plugin-singlefile';

// The routes the radio serves that are not the page itself.
const RADIO_ROUTES = ['/api', '/auth', '/wifi', '/update', '/reboot'];

const MOCK_DIR = path.resolve('mock');

// Answers the API from the files scripts/record-mock.mjs wrote, as the radio
// would. A POST changes nothing and answers the way the radio answers a
// command it took.
function mockRadio() {
  return {
    name: 'mock-radio',
    configureServer(server) {
      // No live state in recorded data: the socket is turned away, so the
      // page polls, as it does when the radio has two pages open already.
      server.httpServer?.on('upgrade', (req, socket) => {
        if (!req.url.startsWith('/api/live')) return;
        // A page closed while it is answered resets the socket; unheard,
        // that error would stop this server.
        socket.on('error', () => {});
        socket.end('HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\n\r\n');
      });
      server.middlewares.use((req, res, next) => {
        const url = new URL(req.url, 'http://radio');
        if (!RADIO_ROUTES.some((r) => url.pathname.startsWith(r))) {
          return next();
        }
        if (req.method !== 'GET') {
          res.setHeader('Content-Type', 'text/plain');
          res.end('ok (mock: nothing was sent to a radio)\n');
          return;
        }
        const index = JSON.parse(fs.readFileSync(path.join(MOCK_DIR, 'index.json'), 'utf8'));
        const entry = index[url.pathname];
        if (!entry) {
          res.statusCode = 404;
          res.setHeader('Content-Type', 'text/plain');
          res.end(`No recording of ${url.pathname}. Run npm run record with the radio on.\n`);
          return;
        }
        res.setHeader('Content-Type', entry.type);
        res.end(fs.readFileSync(path.join(MOCK_DIR, entry.file)));
      });
    },
  };
}

// Preact's JSX, not React's.
const JSX = { runtime: 'automatic', importSource: 'preact' };

export default defineConfig(({ mode }) => {
  // Only RADIO is read from the environment and from .env files, so no other
  // value in them can reach this config.
  const env = loadEnv(mode, process.cwd(), 'RADIO');
  const radio = env.RADIO || 'http://tef668x.lan:8080';
  const live = mode === 'radio';
  return {
    plugins: [tailwindcss(), viteSingleFile(), ...(live ? [] : [mockRadio()])],
    oxc: { jsx: JSX },
    // The dev server's first scan of the imports has its own JSX setting.
    optimizeDeps: { rolldownOptions: { transform: { jsx: JSX } } },
    server: {
      proxy: live
        ? Object.fromEntries(RADIO_ROUTES.map((r) => [r, { target: radio, changeOrigin: true, ws: r === '/api' }]))
        : undefined,
    },
    build: { target: 'es2020' },
  };
});
