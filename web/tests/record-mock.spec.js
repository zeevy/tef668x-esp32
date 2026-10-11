// The recorder hides what is private before it saves: it records from a
// made up radio, with a PC on the PC Link and a log note, into a folder of
// its own, and nothing private is left in the files.
import fs from 'node:fs';
import http from 'node:http';
import path from 'node:path';
import { promisify } from 'node:util';
import { execFile } from 'node:child_process';
import { expect, test } from '@playwright/test';

const ANSWERS = {
  '/auth': ['text/plain', 'ok'],
  '/api/state': ['application/json', JSON.stringify({ net: 'ap', apn: 'tef668x-setup-9F3C', ip: '10.20.30.7', pcl: { on: true, cli: 1, ips: ['10.20.30.99'], ref: 0 } })],
  '/api/settings': ['application/json', JSON.stringify({ sid: 'Home "Net", 5G', pss: true, cst: ['#141415'] })],
  '/api/log': ['application/x-ndjson', '{"n":2}\n{"id":1,"rt":"News, at 10.20.30.40","note":"at uncle\'s house"}\n{"id":2,"rt":null,"note":null}\n'],
  '/api/log.csv': ['text/csv', 'time,real,band,khz,name,pi,rt,note,renamed\n2026-10-11 09:00,1,FM,106400,MAGIC,1064,"Hits, all day","near ""home"", Pune",0\n2026-10-11 09:05,1,FM,93500,,,,,0\n'],
  '/api/presets.csv': ['text/csv', 'slot,band,frequency,bandwidth,name,pi\n1,FM,92700,0,BIG FM,\n'],
};

test('the recorder blanks names, addresses, PCs and notes', async ({}, testInfo) => {
  const radio = http.createServer((req, res) => {
    const answer = ANSWERS[new URL(req.url, 'http://radio').pathname];
    if (!answer) return res.writeHead(404).end();
    res.writeHead(200, { 'Content-Type': answer[0], 'Set-Cookie': 'tef=1; Path=/' }).end(answer[1]);
  });
  await new Promise((done) => radio.listen(0, '127.0.0.1', done));
  const dir = testInfo.outputPath();
  fs.mkdirSync(path.join(dir, 'mock'), { recursive: true });
  const script = new URL('../scripts/record-mock.mjs', import.meta.url).pathname;
  const { stdout } = await promisify(execFile)('node', [script], { cwd: dir, env: { ...process.env, RADIO: `http://127.0.0.1:${radio.address().port}` } });
  radio.close();
  const read = (file) => fs.readFileSync(path.join(dir, 'mock', file), 'utf8');

  const all = ['state.json', 'settings.json', 'log.ndjson', 'log.csv', 'presets.csv'].map(read).join('\n');
  for (const secret of ['10.20.30', 'Home', '9F3C', 'uncle', 'Pune']) expect(all).not.toContain(secret);

  const state = JSON.parse(read('state.json'));
  expect(state.ip).toBe('192.168.1.40');
  expect(state.apn).toBe('tef668x-setup-A1B2');
  expect(state.pcl).toEqual({ on: true, cli: 1, ips: [], ref: 0 });
  expect(JSON.parse(read('settings.json')).sid).toBe('MyNetwork');
  const log = read('log.ndjson').trim().split('\n').map((l) => JSON.parse(l));
  expect(log[1]).toEqual({ id: 1, rt: 'News, at 192.168.1.41', note: null });
  expect(read('log.csv')).toBe('time,real,band,khz,name,pi,rt,note,renamed\n2026-10-11 09:00,1,FM,106400,MAGIC,1064,"Hits, all day",,0\n2026-10-11 09:05,1,FM,93500,,,,,0\n');
  expect(read('presets.csv')).toBe(ANSWERS['/api/presets.csv'][1]);
  expect(stdout).toContain('Read git diff web/mock before a commit');
});
