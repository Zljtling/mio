import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawn } from 'node:child_process';

const folder = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(folder, '../..');
const ffmpeg = 'C:\\Program Files\\JianyingPro\\Apps\\5.7.0.11527\\ffmpeg.exe';
const start = Number(process.argv[2] ?? 0);
const count = Number(process.argv[3] ?? 2160);
const output = path.join(folder, process.argv[4] ?? 'JOJO游戏演示-无声.mp4');
if (!Number.isInteger(start) || !Number.isInteger(count) || count <= 0) throw Error('Invalid frame range');

const renderer = spawn(path.join(folder, 'render_demo.exe'), [String(start), String(count)], {
  cwd: root, windowsHide: true, stdio: ['ignore', 'pipe', 'pipe']
});
const encoder = spawn(ffmpeg, [
  '-hide_banner', '-y', '-loglevel', 'warning', '-stats',
  '-f', 'rawvideo', '-pixel_format', 'bgra', '-video_size', '1600x900', '-framerate', '24', '-i', 'pipe:0',
  '-vf', 'scale=1280:720:flags=lanczos,format=yuv420p',
  '-c:v', 'h264_mf', '-b:v', '6000k', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', '-an', output
], { cwd: root, windowsHide: true, stdio: ['pipe', 'ignore', 'pipe'] });

renderer.stdout.pipe(encoder.stdin);
renderer.stderr.on('data', data => process.stderr.write(`renderer: ${data}`));
encoder.stderr.on('data', data => process.stderr.write(String(data)));
encoder.stdin.on('error', () => {});
const wait = child => new Promise((resolve, reject) => {
  child.once('error', reject);
  child.once('close', code => code === 0 ? resolve() : reject(new Error(`${child.spawnfile} exited ${code}`)));
});
await Promise.all([wait(renderer), wait(encoder)]);
console.log(`Encoded ${count} frames (${(count / 24).toFixed(2)} seconds): ${output}`);
