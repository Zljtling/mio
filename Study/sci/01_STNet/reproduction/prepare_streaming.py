"""Equivalent spot preparation, processing one section at a time to limit RAM."""
import json
import pickle
import gzip
from pathlib import Path

import numpy as np
import pandas as pd
from PIL import Image
from tqdm import tqdm

BASE = Path(__file__).resolve().parent.parent
RAW = BASE / 'data/hist2tscript'
DEST = BASE / 'data/hist2tscript-patch'


def main():
    images = sorted(RAW.glob('*/*/*_*.jpg'))
    assert images, 'Run fetch_data.py first'
    gene_set = set()
    for im in images:
        with gzip.open(im.with_suffix('.tsv.gz'), 'rt') as f:
            gene_set.update(f.readline().rstrip('\r\n').split('\t')[1:])
    genes = sorted(gene_set)
    subtype = {im.parent.name: im.parent.parent.name for im in images}
    DEST.mkdir(parents=True, exist_ok=True)
    for name, value in [('gene.pkl', genes), ('subtype.pkl', subtype)]:
        with (DEST / name).open('wb') as f:
            pickle.dump(value, f)
    summary = []
    for im in tqdm(images, desc='Sections'):
        patient, section = im.stem.split('_')
        dest = DEST / subtype[patient] / patient
        dest.mkdir(parents=True, exist_ok=True)
        counts = pd.read_csv(im.with_suffix('.tsv.gz'), sep='\t', index_col=0)
        counts = counts.reindex(columns=genes, fill_value=0).astype(float)
        spots = pd.read_csv(im.with_suffix('.spots.txt'), sep='\t')
        annotations = pd.read_csv(im.with_name(im.stem + '_Coords.tsv'), sep='\t')
        tumors = {(int(round(r.x)), int(round(r.y))): r.tumor == 'tumor'
                  for r in annotations.itertuples()}
        with Image.open(im) as image:
            width, height = image.size
        n = 0
        for r in spots.itertuples():
            x, y = int(round(r.pixel_x)), int(round(r.pixel_y))
            coord = int(r.x), int(r.y)
            if x - 112 < 0 or y - 112 < 0 or x + 112 > width or y + 112 > height:
                continue
            if coord not in tumors:
                continue
            target = dest / f'{section}_{coord[0]}_{coord[1]}.npz'
            if not target.exists():
                np.savez_compressed(target, count=counts.loc[f'{coord[0]}x{coord[1]}'].to_numpy(),
                                    tumor=tumors[coord], pixel=[x, y], patient=[patient],
                                    section=[section], index=coord)
            n += 1
        (dest / ('.' + section)).touch()
        summary.append(dict(patient=patient, section=section, spots=n, width=width, height=height))
    files = sorted(DEST.glob('*/*/*_*_*.npz'))
    # Same mean/median as upstream's dense gene x spot matrix, using disk backing.
    scratch = DEST / 'counts-statistics.tmp.npy'
    matrix = np.lib.format.open_memmap(scratch, mode='w+', dtype=np.float64,
                                      shape=(len(genes), len(files)))
    for start in tqdm(range(0, len(files), 256), desc='Count statistics'):
        columns = []
        for filename in files[start:start + 256]:
            with np.load(filename) as data:
                columns.append(data['count'])
        matrix[:, start:start + len(columns)] = np.stack(columns, axis=1)
    matrix.flush()
    mean, median = np.empty(len(genes)), np.empty(len(genes))
    for start in range(0, len(genes), 256):
        block = np.array(matrix[start:start + 256])
        mean[start:start + len(block)] = np.mean(block, axis=1)
        median[start:start + len(block)] = np.median(block, axis=1)
    np.save(DEST / 'mean_expression.npy', mean)
    np.save(DEST / 'median_expression.npy', median)
    del matrix
    scratch.unlink()
    report = dict(patients=len(subtype), sections=len(images), spots=len(files),
                  genes=len(genes), samples=summary)
    reports = BASE / 'reproduction/reports'
    reports.mkdir(parents=True, exist_ok=True)
    (reports / 'dataset-summary.json').write_text(json.dumps(report, indent=2))
    print(json.dumps({k: v for k, v in report.items() if k != 'samples'}), flush=True)


if __name__ == '__main__':
    main()
