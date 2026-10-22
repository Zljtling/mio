"""Nature-figure / Python: spatial expression, held-out performance and learning curves.

All observations are retained. FASN and all held-out sections are fixed before
plotting; one patient, one seed, no inferential CI or independent-spot claims.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import zipfile

os.environ.setdefault('OMP_NUM_THREADS', '2')
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.colors import Normalize
from matplotlib.lines import Line2D
import numpy as np
import pandas as pd
from PIL import Image

BASE = Path(__file__).resolve().parents[2]
SKILL = Path(os.environ.get('NATURE_FIGURE_SKILL', str(Path.home() / '.agents/skills/nature-figure')))
sys.path.insert(0, str(SKILL / 'scripts'))
from audit_panel_alignment import require_matplotlib_panel_alignment

TEAL, GREY = '#287D8E', '#91999E'
plt.rcParams.update({
    'font.family': 'sans-serif', 'font.sans-serif': ['Arial', 'Helvetica', 'DejaVu Sans'],
    'font.size': 7, 'axes.titlesize': 7, 'axes.labelsize': 7,
    'xtick.labelsize': 6, 'ytick.labelsize': 6, 'legend.fontsize': 6,
    'svg.fonttype': 'none', 'pdf.fonttype': 42,
    'axes.spines.top': False, 'axes.spines.right': False,
    'axes.linewidth': .65, 'lines.linewidth': 1.0, 'legend.frameon': False,
    'savefig.facecolor': 'white', 'figure.facecolor': 'white',
})


def correlations(a, b):
    a, b = a - a.mean(0), b - b.mean(0)
    den = np.sqrt(np.sum(a*a, 0) * np.sum(b*b, 0))
    return np.divide(np.sum(a*b, 0), den, out=np.full(den.shape, np.nan), where=den > 0)


def complete_predictions(folder, last=50):
    results = []
    for p in folder.glob('pred_*.npz'):
        match = re.fullmatch(r'pred_(\d+)\.npz', p.name)
        if match and int(match[1]) <= last:
            try:
                with zipfile.ZipFile(p) as f:
                    if f.testzip() is None:
                        results.append((int(match[1]), p))
            except (OSError, zipfile.BadZipFile):
                pass  # A currently written file is deferred, never interpreted as missing observations.
    return sorted(results)


def label(ax, name):
    ax.text(-.13, 1.07, name, transform=ax.transAxes, fontsize=8, fontweight='bold', va='bottom')


def export(fig, axes, stem, out, *, rows=None, columns=None):
    qa = out / 'qa'
    qa.mkdir(exist_ok=True)
    fig.canvas.draw()
    require_matplotlib_panel_alignment(
        fig, axes=axes, panel_ids=[chr(97+i) for i in range(len(axes))],
        row_groups=rows, column_groups=columns, tolerance_pt=1.5,
        gutter_tolerance_pt=1.5, strict=True,
        json_out=qa / f'{stem}.alignment.json',
        overlay_svg=qa / f'{stem}.alignment.svg')
    for suffix in ['pdf', 'svg']:
        fig.savefig(out / f'{stem}.{suffix}', dpi=600)
    fig.savefig(out / f'{stem}.png', dpi=600)
    plt.close(fig)
    for script, extra, filename in [
        ('audit_pdf_text.py', ['--min-pt', '5', '--json'], f'{stem}.text.json'),
        ('audit_figure_collisions.py', ['--json-out', str(qa / f'{stem}.collision.json'),
                                      '--overlay-pdf', str(qa / f'{stem}.collision.pdf')], f'{stem}.collision.txt'),
    ]:
        result = subprocess.run([sys.executable, str(SKILL / 'scripts' / script),
                                 str(out / f'{stem}.pdf'), *extra], capture_output=True, text=True)
        (qa / filename).write_text(result.stdout + result.stderr)
        if result.returncode:
            raise RuntimeError(f'{stem}: {script} blocked delivery; inspect {qa / filename}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prediction', type=Path)
    parser.add_argument('--final', action='store_true')
    parser.add_argument('--out', type=Path)
    parser.add_argument('--png-only', action='store_true', help='Keep PNG figures; discard vector and QA-overlay graphics after auditing')
    args = parser.parse_args()
    predictions = complete_predictions(BASE / 'output/simplified')
    assert predictions, 'No complete predictions yet'
    prediction = args.prediction.resolve() if args.prediction else predictions[-1][1]
    epoch = int(re.fullmatch(r'pred_(\d+)\.npz', prediction.name)[1])
    if args.final:
        assert epoch == 50, 'Final figures must use the prespecified epoch 50'
        status = json.loads((prediction.parent / 'status.json').read_text())
        assert status['status'] == 'completed', 'Training must finish before final rendering'
    out = args.out or BASE / 'output/figures' / ('final_epoch_050' if args.final else f'preview_epoch_{epoch:03d}')
    out.mkdir(parents=True, exist_ok=True)
    (out / 'qa').mkdir(exist_ok=True)
    preflight = subprocess.run([sys.executable, str(SKILL / 'scripts/validate_figure.py'),
                               str(Path(__file__).resolve()), '--json'], capture_output=True, text=True)
    (out / 'qa/source-preflight.json').write_text(preflight.stdout + preflight.stderr)
    if preflight.returncode:
        raise RuntimeError('Plotting source preflight failed')
    (out / 'source_data').mkdir(exist_ok=True)
    source = out / 'source_data'
    with np.load(prediction) as f:
        data = {k: f[k] for k in f.files}
    true, pred = data['counts'].astype(float), data['predictions'].astype(float)
    assert true.shape == pred.shape and true.shape[1] == 250
    assert np.isfinite(true).all() and np.isfinite(pred).all()
    assert len(np.unique(data['patient'])) == 1
    patient = str(data['patient'][0])
    gene_names = data['gene_names']
    matches = np.flatnonzero(gene_names == 'FASN')
    assert len(matches) == 1, 'Prespecified FASN not found'
    j = int(matches[0])
    sections = sorted(np.unique(data['section']))
    assert len(sections) == 3
    r = correlations(pred, true)
    baseline = np.asarray(data['mean_expression']).reshape(-1)
    model_rmse = np.sqrt(np.mean((pred-true)**2, 0))
    base_rmse = np.sqrt(np.mean((baseline-true)**2, 0))
    pd.DataFrame(dict(gene=gene_names, ensembl=data['ensg_names'], pearson=r,
                      model_rmse=model_rmse, train_mean_rmse=base_rmse)).to_csv(source / 'all_genes.csv', index=False)
    mean, sd = float(true[:, j].mean()), float(true[:, j].std())
    assert sd > 0
    z_true, z_pred = (true[:, j]-mean)/sd, (pred[:, j]-mean)/sd
    pd.DataFrame(dict(patient=data['patient'], section=data['section'],
                      pixel_x=data['pixel'][:, 0], pixel_y=data['pixel'][:, 1],
                      tumor=data['tumor'].reshape(-1), true_FASN=true[:, j], pred_FASN=pred[:, j],
                      true_z=z_true, pred_z=z_pred)).to_csv(source / 'spatial_FASN.csv', index=False)
    badge = f'FINAL: epoch 50/50' if args.final else f'PREVIEW: epoch {epoch}/50; training incomplete'
    title = f'{patient} | 1 held-out patient | {len(true)} spots | {badge}'

    # Figure 1: complete field and all sections, matching the evidence roles of paper Fig. 2a.
    images, sizes, raw_files = [], [], []
    for section in sections:
        candidates = list((BASE / 'data/hist2tscript').glob(f'*/{patient}/{patient}_{section}.jpg'))
        assert len(candidates) == 1
        with Image.open(candidates[0]) as im:
            sizes.append(im.size)
            im.thumbnail((1100, 1100), Image.Resampling.LANCZOS)
            images.append(im.copy())
        raw_files.append(str(candidates[0].relative_to(BASE)))
    length = max(max(s) for s in sizes)
    fig, grid = plt.subplots(3, 4, figsize=(183/25.4, 165/25.4))
    fig.subplots_adjust(left=.045, right=.99, top=.905, bottom=.14, wspace=.13, hspace=.22)
    fig.suptitle(title, fontsize=7, y=.975)
    headers = ['H&E image', 'Tumour annotation', 'True FASN', 'Predicted FASN']
    for row, section in enumerate(sections):
        keep = data['section'] == section
        x, y = data['pixel'][keep].T
        for col in range(4):
            ax = grid[row, col]
            ax.set_xlim(0, length)
            ax.set_ylim(length, 0)
            ax.set_aspect('equal')
            ax.set_xticks([]); ax.set_yticks([])
            for spine in ax.spines.values(): spine.set_visible(False)
            if col == 0:
                width, height = sizes[row]
                ax.imshow(images[row], extent=(0, width, height, 0), interpolation='nearest')
                ax.plot([400, 1400], [length-400, length-400], color='#222222', lw=1.1)
                ax.text(400, length-850, '1,000 px', fontsize=5.5, va='bottom')
                ax.set_ylabel(f'{section}  |  n = {keep.sum()}', fontsize=6, labelpad=7)
            elif col == 1:
                tumour = data['tumor'].reshape(-1)[keep].astype(bool)
                ax.scatter(x[~tumour], y[~tumour], s=6, c='#D8DADD', linewidths=0, rasterized=True)
                ax.scatter(x[tumour], y[tumour], s=6, c='#292F33', linewidths=0, rasterized=True)
            else:
                values = z_true[keep] if col == 2 else z_pred[keep]
                ax.scatter(x, y, c=values, cmap='RdBu_r', norm=Normalize(-3, 3), s=7,
                           linewidths=0, rasterized=True)
            if row == 0: ax.set_title(headers[col], pad=12)
            label(ax, chr(97+row*4+col))
    cax = fig.add_axes([.55, .085, .33, .012])
    cb = fig.colorbar(matplotlib.cm.ScalarMappable(norm=Normalize(-3, 3), cmap='RdBu_r'), cax=cax, orientation='horizontal')
    cb.set_ticks([-3, 0, 3]); cb.set_label('FASN z-score (shared truth reference)', fontsize=6, labelpad=3)
    fig.text(.06, .083, 'Dark: tumour   |   Light: non-tumour', fontsize=6)
    fig.text(.06, .025, 'All three sections shown. Pixel scale only; no micrometre calibration assumed.', fontsize=6)
    export(fig, list(grid.flat), 'fig1_spatial_FASN', out)

    # Figure 2: distribution -> credible paired control -> concrete calibration boundary.
    fig, axs = plt.subplots(1, 3, figsize=(183/25.4, 69/25.4))
    fig.subplots_adjust(left=.075, right=.97, bottom=.25, top=.74, wspace=.55)
    fig.suptitle(title, fontsize=7, y=.96)
    finite = np.isfinite(r)
    axs[0].hist(r[finite], bins=np.linspace(-1, 1, 21), color=TEAL, edgecolor='white', linewidth=.3)
    axs[0].axvline(0, color=GREY, lw=.75, ls='--')
    axs[0].set(xlabel='Pearson r', ylabel='Genes', xlim=(-1, 1))
    axs[0].set_xticks([-1, 0, 1]); axs[0].set_title(f'All genes: {np.sum(r > 0)}/250 positive', pad=10)
    axs[1].scatter(base_rmse, model_rmse, s=6, color=TEAL, alpha=.65, linewidths=0, rasterized=True)
    lo, hi = 0., float(max(base_rmse.max(), model_rmse.max())*1.05)
    axs[1].plot([lo, hi], [lo, hi], ls='--', lw=.75, color=GREY)
    axs[1].set(xlim=(lo, hi), ylim=(lo, hi), xlabel='Training-mean RMSE', ylabel='ST-Net RMSE')
    axs[1].set_title('Matched genes; below line is better', pad=10)
    axs[2].scatter(true[:, j], pred[:, j], s=5, color=TEAL, alpha=.4, linewidths=0, rasterized=True)
    lo, hi = float(min(true[:, j].min(), pred[:, j].min())), float(max(true[:, j].max(), pred[:, j].max()))
    pad = (hi-lo)*.05; lo -= pad; hi += pad
    axs[2].plot([lo, hi], [lo, hi], ls='--', lw=.75, color=GREY)
    axs[2].set(xlim=(lo, hi), ylim=(lo, hi), xlabel='True log expression', ylabel='Predicted log expression')
    axs[2].set_title(f'FASN: r = {r[j]:.3f}', pad=10)
    for i, ax in enumerate(axs): label(ax, chr(97+i))
    fig.text(.075, .04, 'One patient / one seed. Spots and genes are not independent biological replicates.', fontsize=6)
    export(fig, list(axs), 'fig2_heldout_performance', out)

    # Figure 3: diagnostics, never selection on the held-out test patient.
    losses = {}
    mode, current = None, None
    for line in (prediction.parent / 'train.log').read_text().splitlines():
        match = re.search(r'Epoch #(\d+)', line)
        if match: current = int(match[1])
        if line.endswith(' train:'): mode = 'train'
        if line.endswith(' test:'): mode = 'test'
        match = re.search(r'Loss:\s+([\d.eE+-]+)', line)
        if match and mode and current is not None and current <= epoch:
            losses.setdefault(current, {})[mode] = float(match[1])
    history = []
    for ep, file in complete_predictions(prediction.parent, epoch):
        with np.load(file) as f:
            assert np.array_equal(f['gene_names'], gene_names)
            rr = correlations(f['predictions'].astype(float), f['counts'].astype(float))
        history.append(dict(epoch=ep, mean_pearson=float(np.nanmean(rr)), median_pearson=float(np.nanmedian(rr)),
                            train_mse=losses.get(ep, {}).get('train', np.nan), test_mse=losses.get(ep, {}).get('test', np.nan)))
    archived_history = BASE / 'output/simplified/figure_source/learning_history.csv'
    if archived_history.exists():
        archived = pd.read_csv(archived_history)
        archived = archived[archived.epoch <= epoch]
        complete_history = {int(row['epoch']): row for row in archived.to_dict('records')}
        complete_history.update({row['epoch']: row for row in history})
        history = [complete_history[e] for e in sorted(complete_history)]
    hist = pd.DataFrame(history)
    hist.to_csv(source / 'learning_history.csv', index=False)
    fig, axs = plt.subplots(1, 2, figsize=(183/25.4, 72/25.4))
    fig.subplots_adjust(left=.09, right=.97, bottom=.27, top=.76, wspace=.40)
    fig.suptitle(title, fontsize=7, y=.96)
    axs[0].plot(hist.epoch, hist.train_mse, color=GREY, label='Train')
    axs[0].plot(hist.epoch, hist.test_mse, color=TEAL, label='Held-out test')
    axs[0].set(xlabel='Epoch', ylabel='Mean squared error', title='Optimisation and generalisation')
    axs[1].plot(hist.epoch, hist.mean_pearson, color=GREY, label='Mean')
    axs[1].plot(hist.epoch, hist.median_pearson, color=TEAL, label='Median')
    axs[1].set(xlabel='Epoch', ylabel='Pearson r across genes', title='Spatial association during training')
    for i, ax in enumerate(axs):
        ax.legend(loc='upper center', bbox_to_anchor=(.5, -.27), ncol=2, handlelength=2)
        label(ax, chr(97+i))
    fig.text(.09, .025, 'Test curves are diagnostic only. The final reported epoch is fixed at 50.', fontsize=6)
    export(fig, list(axs), 'fig3_learning_curves', out)
    manifest = dict(skill='nature-figure', skill_version='2.8.0', backend='Python/Matplotlib',
                    stage='final' if args.final else 'preview', epoch=epoch,
                    prediction=str(prediction.relative_to(BASE)), sha256=hashlib.sha256(prediction.read_bytes()).hexdigest(),
                    patient=patient, biological_patients=1, seeds=1, spots=len(true), genes=true.shape[1],
                    section_counts={str(s):int(np.sum(data['section']==s)) for s in sections},
                    exclusions={'spots':0,'genes':0,'undefined_Pearson':int(np.sum(~finite))},
                    gene='FASN', gene_selection='Prespecified from the paper, not selected by held-out performance',
                    display_transform=dict(reference='Held-out true FASN mean and population SD, shared by prediction',
                                           mean=mean, sd=sd, color_limits=[-3,3],
                                           true_clipped=int(np.sum(np.abs(z_true)>3)), pred_clipped=int(np.sum(np.abs(z_pred)>3))),
                    images=raw_files, image_processing='Full image, aspect preserved, display thumbnail only, no contrast/gamma edits; square white padding',
                    calibration='Pixels only; no assumed micrometre scale',
                    statistics='Descriptive only; no biological CI or significance tests with one patient',
                    reference='Local paper Fig. 2a; no external-data AUROC or 23-patient consistency panel')
    (out / 'figure_manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2))
    (out / 'qa/review-notes.md').write_text(
        '# QA interpretation\n\n'
        'Alignment, PDF glyph size and rendered collisions were audited for each exported figure. '
        'The source uncertainty warning is intentional: one seed and one biological patient do not support '
        'a seed/patient confidence interval. Gene-wise means and medians are descriptive, not replicate estimates. '
        'No confidence intervals, P values or extra biological replicates have been invented.\n\n'
        'The raster-export warning is reviewed: this bundle intentionally provides editable PDF/SVG '
        'plus 600-dpi PNG previews, not a TIFF submission package or a claim of journal acceptance readiness.\n\n'
        'Figure 1: rows are all three technical sections; raw image and annotation localise the spatial evidence; '
        'shared truth-referenced colour scaling preserves prediction bias/underdispersion. Pixel scale labels '
        'are intentional image overlays. No image crop or local contrast edit is applied.\n\n'
        'Figure 2: all 250 genes and all held-out FASN spots are plotted. Identity lines denote equal values, '
        'not fitted regressions. Undefined correlations, if present, are counted in the manifest.\n\n'
        'Figure 3: training trajectories use all complete saved epochs through the plotted epoch. '
        'No test-based epoch selection is performed.\n\n'
        'Human visual review must inspect the complete figure and each panel after final rendering. '
        'Automated checks alone do not certify scientific or visual correctness.\n')
    (out / 'captions.md').write_text(
        f'# Figure captions — {badge}\n\n'
        f'**Figure 1 | Spatial FASN prediction in held-out patient {patient}.** '
        'Rows show all three sections; columns show full H&E images, tumour annotation, true FASN and ST-Net prediction. '
        'True and predicted values share the true-expression z-score reference and colour limits. '
        'Clipping affects display colours only; raw values and counts are in the source data and manifest. '
        'Image aspect is preserved; scale bars are in source pixels, not micrometres.\n\n'
        '**Figure 2 | Held-out performance and calibration.** '
        'a, Pearson correlation distribution across all 250 genes. b, Paired per-gene RMSE against the training-mean predictor; '
        'the dashed identity line indicates equal error. c, All tested spots for prespecified FASN with an identity line. '
        'No confidence intervals or significance tests are reported: this is one patient and one random seed, '
        'and spots are not independent biological replicates.\n\n'
        '**Figure 3 | Learning diagnostics.** Training/test mean squared error and descriptive gene-wise Pearson summaries '
        'over saved epochs. Held-out curves do not select the reporting epoch; epoch 50 is fixed in advance.\n')
    if args.png_only:
        for suffix in ['*.pdf', '*.svg']:
            for file in out.glob(suffix):
                file.unlink()
            for file in (out / 'qa').glob(suffix):
                file.unlink()
    print(out)


if __name__ == '__main__':
    main()
