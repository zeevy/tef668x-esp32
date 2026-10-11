// The built page's size, as the radio will store it: gzipped.
import fs from 'node:fs';
import zlib from 'node:zlib';

const html = fs.readFileSync('dist/index.html');
const gz = zlib.gzipSync(html, { level: 9 });
console.log(`dist/index.html: ${html.length} bytes, ${gz.length} gzipped`);
