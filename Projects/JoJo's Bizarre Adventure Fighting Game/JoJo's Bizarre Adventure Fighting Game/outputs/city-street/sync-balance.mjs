import fs from 'node:fs/promises';
import { FileBlob, SpreadsheetFile } from '@oai/artifact-tool';
const filename = new URL('../naiwa-balance/角色数值.xlsx', import.meta.url).pathname.replace(/^\//, '');
const workbook = await SpreadsheetFile.importXlsx(await FileBlob.load(decodeURI(filename)));
const sheet = workbook.worksheets.getItem('角色数值');
console.log((await workbook.inspect({kind:'workbook,sheet,table',maxChars:1600,tableMaxRows:2,tableMaxCols:5})).ndjson);
const rows = sheet.getRange('A6:E56').values;
for (let i = 0; i < rows.length; i++) {
  const [character,key] = rows[i];
  if ((character === 'kakyoin' && key === 'special_mp_gain') || (character === 'naiwa' && key === 'passive_mp_per_hit'))
    sheet.getRange(`C${i+6}`).values = [[2]];
}
sheet.tables.items[0].rows.add(null, [['naiwa','transformed_run_speed',0.15,'px/ms','被大招变身后的水平移动速度']]);
sheet.getRange('A57:E57').copyFrom(sheet.getRange('A56:E56'),'all');
sheet.getRange('A57:E57').values = [['naiwa','transformed_run_speed',0.15,'px/ms','被大招变身后的水平移动速度']];
sheet.getRange('C57').setNumberFormat('0.00');
workbook.recalculate();
const preview = await workbook.render({sheetName:'角色数值',range:'A50:E57',scale:1,format:'png'});
await fs.writeFile(new URL('./balance-after.png',import.meta.url),new Uint8Array(await preview.arrayBuffer()));
console.log((await workbook.inspect({kind:'region',sheetId:'角色数值',range:'A55:E57',maxChars:1600})).ndjson);
await (await SpreadsheetFile.exportXlsx(workbook)).save(decodeURI(filename));
