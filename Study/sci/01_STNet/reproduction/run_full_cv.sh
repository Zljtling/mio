#!/usr/bin/env bash
set -euo pipefail
BASE="$(cd "$(dirname "$0")/.." && pwd)"
source /home/mio/tools/miniconda3/etc/profile.d/conda.sh
conda activate STNet
export OMP_NUM_THREADS=4
export MKL_NUM_THREADS=4
export TORCH_HOME="$BASE/data/torch"
cd "$BASE/ST-Net"
mapfile -t patients < <(python -m stnet patients)
test "${#patients[@]}" -eq 23
mkdir -p "$BASE/output/full_cv"
# Optional arguments select outer held-out patients; default: all 23.
if [ "$#" -gt 0 ]; then patients=("$@"); fi
for patient in "${patients[@]}"; do
    echo "Outer test patient: $patient"
    python bin/cross_validate.py "$BASE/output/full_cv/${patient}_" 4 50 "$patient" \
        --lr 1e-6 --window 224 --model densenet121 --pretrained --average \
        --batch 32 --workers 4 --gene_n 250 --norm --gpu --restart --loglevel INFO
done
