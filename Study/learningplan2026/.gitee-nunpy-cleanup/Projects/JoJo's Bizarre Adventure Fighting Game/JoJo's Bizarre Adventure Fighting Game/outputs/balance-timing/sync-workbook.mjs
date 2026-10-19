import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { FileBlob, SpreadsheetFile } from '@oai/artifact-tool';

const dir = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(dir, '../..');
const workbookPath = path.join(root, 'outputs/naiwa-balance/角色数值.xlsx');
const csvBytes = await fs.readFile(path.join(root, 'resources/character_stats.csv'));
const csv = new TextDecoder('gb18030').decode(csvBytes);
const csvRows = csv.trimEnd().split(/\r?\n/).slice(1).map(line => line.split(','));
if (!csvRows.every(row => row.length === 5)) throw new Error('CSV columns changed; stop before altering the workbook');
const expected = new Map(csvRows.map(row => [`${row[0]}.${row[1]}`, row]));
if (expected.size !== csvRows.length) throw new Error('Duplicate character/key in CSV');

const workbook = await SpreadsheetFile.importXlsx(await FileBlob.load(workbookPath));
const sheet = workbook.worksheets.getItem('角色数值');
const table = sheet.tables.items[0];
const oldRows = sheet.getRange(`A6:E${5 + csvRows.length}`).values.filter(row => row[0]);
const seen = new Set();
for (let index = 0; index < oldRows.length; index++) {
  const row = oldRows[index];
  const id = `${row[0]}.${row[1]}`;
  const current = expected.get(id);
  if (!current) throw new Error(`Workbook-only row ${id}; review before synchronizing`);
  seen.add(id);
  const excelRow = index + 6;
  sheet.getRange(`A${excelRow}:E${excelRow}`).values = [[
    current[0], current[1], Number(current[2]), current[3], current[4]
  ]];
  if (current[1].endsWith('_ms')) sheet.getRange(`C${excelRow}`).setNumberFormat('0');
}

const additions = csvRows.filter(row => !seen.has(`${row[0]}.${row[1]}`));
for (let index = 0; index < additions.length; index++) {
  const row = additions[index];
  const excelRow = 6 + oldRows.length + index;
  const values = [[row[0], row[1], Number(row[2]), row[3], row[4]]];
  table.rows.add(null, values);
  sheet.getRange(`A${excelRow}:E${excelRow}`).copyFrom(sheet.getRange('A57:E57'), 'all');
  sheet.getRange(`A${excelRow}:E${excelRow}`).values = values;
  sheet.getRange(`C${excelRow}`).setNumberFormat('0');
}

workbook.recalculate();
const lastRow = 5 + oldRows.length + additions.length;
const inspected = await workbook.inspect({kind: 'region', sheetId: '角色数值', range: `A${Math.max(6, lastRow - 6)}:E${lastRow}`, maxChars: 1800});
console.log(inspected.ndjson);
const preview = await workbook.render({sheetName: '角色数值', range: 'A57:E64', scale: 1, format: 'png'});
await fs.writeFile(path.join(dir, 'timing-rows-preview.png'), new Uint8Array(await preview.arrayBuffer()));
await (await SpreadsheetFile.exportXlsx(workbook)).save(workbookPath);
console.log(`Synchronized ${oldRows.length} original rows and added ${additions.length} timing rows to ${workbookPath}`);
