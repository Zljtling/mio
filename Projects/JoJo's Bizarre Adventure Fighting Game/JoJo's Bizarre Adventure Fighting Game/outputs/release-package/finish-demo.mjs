import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawn } from 'node:child_process';
import fs from 'node:fs/promises';

const folder = path.dirname(fileURLToPath(import.meta.url));
const resources = path.resolve(folder, '../../resources');
const delivery = path.join(folder, '交付');
await fs.mkdir(delivery, { recursive: true });
const inputs = [
  path.join(folder, 'JOJO游戏演示-无声.mp4'),
  ...['bgm_menu.mp3', 'bgm_game.mp3', 'kakyoin_emerald_cannon_cast.wav',
    'kakyoin_emerald_cannon_loop.wav', 'jotaro_charged_punch.wav',
    'jotaro_time_stop.wav', 'jotaro_ora_barrage.wav'].map(name => path.join(resources, name))
];
const filters = [
  '[1:a]atrim=0:15,asetpts=PTS-STARTPTS,volume=0.30,afade=t=out:st=13:d=2[menu]',
  '[2:a]atrim=0:75,asetpts=PTS-STARTPTS,volume=0.25,afade=t=in:st=0:d=1,afade=t=out:st=72:d=3[game]',
  '[menu][game]concat=n=2:v=0:a=1[music]',
  '[3:a]volume=0.6,adelay=46250|46250[cast]',
  '[4:a]volume=0.35,afade=t=out:st=2:d=1,adelay=46800|46800[cannon]',
  '[5:a]volume=0.6,adelay=63960|63960[punch]',
  '[6:a]volume=0.65,adelay=69583|69583[stop]',
  '[7:a]volume=0.55,adelay=70667|70667[ora]',
  '[music][cast][cannon][punch][stop][ora]amix=inputs=6:duration=first:dropout_transition=0:normalize=0,alimiter=limit=0.95:level=false[audio]'
].join(';');
const output = path.join(delivery, 'JOJO格斗游戏-90秒Demo.mp4');
const encoder = spawn('C:\\Program Files\\JianyingPro\\Apps\\5.7.0.11527\\ffmpeg.exe', [
  '-hide_banner', '-y', '-loglevel', 'warning', '-stats',
  ...inputs.flatMap(input => ['-i', input]),
  '-filter_complex', filters, '-map', '0:v:0', '-map', '[audio]',
  '-c:v', 'copy', '-c:a', 'aac', '-b:a', '192k', '-t', '90', '-movflags', '+faststart', output
], { windowsHide: true, stdio: ['ignore', 'ignore', 'inherit'] });
await new Promise((resolve, reject) => {
  encoder.once('error', reject);
  encoder.once('close', code => code === 0 ? resolve() : reject(Error(`FFmpeg exited ${code}`)));
});
for (const name of ['JOJO格斗游戏安装程序.exe', 'JoJoFightingGame-portable.zip']) {
  await fs.copyFile(path.join(folder, name), path.join(delivery, name));
}
await fs.writeFile(path.join(delivery, '使用说明.txt'), '\uFEFF' + [
  'JOJO 格斗游戏 · Windows 64 位版本',
  '',
  '安装：双击 JOJO格斗游戏安装程序.exe，选择目录后点击安装；安装完成后可启动游戏。',
  '免安装：将 JoJoFightingGame-portable.zip 完整解压，双击 启动游戏.cmd。',
  '卸载：安装版本可在 Windows 应用列表中卸载，或运行安装目录下的 Uninstall.exe。',
  '',
  '1P：A/D 移动，W 跳跃，S 格挡（奶娃为瞬移），F 普攻，E 特殊技，Q 大招。',
  '2P：左右方向键移动，上方向键跳跃，下方向键格挡，逗号普攻，句号特殊技，斜杠大招。',
  '选择角色后按 Enter 开始。',
  '',
  '数值调整：修改游戏目录 resources\\character_stats.csv 后重启游戏。',
  '窗口 1600 × 900，城市地图 3200 × 1800。',
  '',
  '视频：90 秒，1280 × 720，24 帧/秒，包含游戏配乐与技能音效。',
  '内容：主菜单与选角、奶娃技能、花京院技能、承太郎技能。',
  '演示由游戏实际代码和素材渲染，使用脚本触发技能，并补充生命与能量以完整展示。',
  ''
].join('\r\n'), 'utf8');
console.log(`Delivery ready: ${delivery}`);
