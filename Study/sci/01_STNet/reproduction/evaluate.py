"""Report RMSE and Pearson correlations on an upstream ST-Net prediction file."""
import argparse
import json
from pathlib import Path
import numpy as np
import pandas as pd
from scipy.stats import pearsonr

parser = argparse.ArgumentParser()
parser.add_argument('prediction', type=Path)
parser.add_argument('--note', default='Pilot metrics only; not the full 23-patient nested cross-validation result.')
args = parser.parse_args()
data = np.load(args.prediction)
pred, true = data['predictions'], data['counts']
assert pred.shape == true.shape and np.isfinite(pred).all() and np.isfinite(true).all()
patients = np.unique(data['patient'])
rows = []
for patient in patients:
    keep = data['patient'] == patient
    for j, name in enumerate(data['gene_names']):
        a, b = pred[keep, j], true[keep, j]
        r = float(pearsonr(a, b).statistic) if np.ptp(a) > 0 and np.ptp(b) > 0 else float('nan')
        rows.append(dict(patient=patient, gene=name, ensembl=data['ensg_names'][j],
                         pearson=r, rmse=float(np.sqrt(np.mean((a-b)**2)))))
table = pd.DataFrame(rows)
table.to_csv(args.prediction.with_suffix('.metrics.csv'), index=False)
baseline = np.asarray(data['mean_expression']).reshape(-1)
summary = dict(prediction=str(args.prediction), patients=patients.tolist(),
               spots=len(pred), genes=pred.shape[1],
               rmse=float(np.sqrt(np.mean((pred-true)**2))),
               train_mean_baseline_rmse=float(np.sqrt(np.mean((baseline-true)**2))),
               median_pearson=float(table.pearson.median()),
               mean_pearson=float(table.pearson.mean()),
               positive_correlations=int((table.pearson > 0).sum()),
               note=args.note)
args.prediction.with_suffix('.summary.json').write_text(json.dumps(summary, indent=2))
print(json.dumps(summary, indent=2))
