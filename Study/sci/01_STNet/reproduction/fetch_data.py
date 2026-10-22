"""Download Mendeley v5 with SHA256 verification and adapt names for upstream ST-Net."""
import argparse
import concurrent.futures
import csv
import gzip
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

HERE = Path(__file__).resolve().parent
BASE = HERE.parent


def fetch(entry):
    target = BASE / 'data' / 'downloads' / entry['filename']
    target.parent.mkdir(parents=True, exist_ok=True)
    expected = entry['content_details']['sha256_hash']
    if target.exists() and hashlib.sha256(target.read_bytes()).hexdigest() == expected:
        return target
    partial = target.with_name(target.name + '.partial')
    subprocess.run(['curl', '-fLsS', '--retry', '4', '--retry-delay', '2',
                    '--connect-timeout', '30', '--max-time', '600',
                    entry['content_details']['download_url'], '-o', str(partial)], check=True)
    assert hashlib.sha256(partial.read_bytes()).hexdigest() == expected, target
    partial.replace(target)
    print('Verified:', target.name, flush=True)
    return target


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--patients', nargs='*', help='default: all 23 patients')
    args = parser.parse_args()
    manifest = json.loads((HERE / 'manifests/dataset-manifest-v5.json').read_text())
    lookup = {f['filename']: f for f in manifest}
    metadata = fetch(lookup['metadata.csv'])
    with metadata.open() as f:
        rows = list(csv.DictReader(f))
    if args.patients:
        rows = [r for r in rows if r['patient'] in args.patients]
    assert rows, 'No matching samples'
    fields = ['count_matrix', 'histology_image', 'spot_coordinates', 'tumor_annotation']
    names = sorted({row[k] for row in rows for k in fields})
    print(f'{len(rows)} sections; {sum(lookup[n]["size"] for n in names)/1e6:.1f} MB', flush=True)
    with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
        list(pool.map(fetch, (lookup[n] for n in names)))
    downloads = BASE / 'data' / 'downloads'
    for row in rows:
        dest = BASE / 'data' / 'hist2tscript' / row['type'] / row['patient']
        dest.mkdir(parents=True, exist_ok=True)
        stem = row['patient'] + '_' + row['replicate']
        for field, suffix in [('count_matrix', '.tsv.gz'), ('histology_image', '.jpg')]:
            target = dest / (stem + suffix)
            if not target.exists():
                target.hardlink_to(downloads / row[field])
        with gzip.open(downloads / row['spot_coordinates'], 'rt') as f:
            spots = list(csv.DictReader(f))
        with (dest / (stem + '.spots.txt')).open('w') as f:
            writer = csv.writer(f, delimiter='\t')
            writer.writerow(['x', 'y', 'pixel_x', 'pixel_y'])
            for spot in spots:
                x, y = spot[''].split('x')
                writer.writerow([int(x), int(y), spot['X'], spot['Y']])
        # v5 has a trailing tab in the header; explicitly retain all five data fields.
        with gzip.open(downloads / row['tumor_annotation'], 'rt') as f:
            annotations = list(csv.reader(f, delimiter='\t'))[1:]
        with (dest / (stem + '_Coords.tsv')).open('w') as f:
            writer = csv.writer(f, delimiter='\t')
            writer.writerow(['spot', 'x', 'y', 'label', 'tumor'])
            writer.writerows(r[:5] for r in annotations if r)
    shutil.copy2(metadata, BASE / 'data' / 'metadata.csv')
    print('Ready:', len(rows), 'sections in', BASE / 'data' / 'hist2tscript', flush=True)


if __name__ == '__main__':
    main()
