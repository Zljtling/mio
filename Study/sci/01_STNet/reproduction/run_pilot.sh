#!/usr/bin/env bash
set -euo pipefail
BASE="$(cd "$(dirname "$0")/.." && pwd)"
source /home/mio/tools/miniconda3/etc/profile.d/conda.sh
conda activate STNet
export OMP_NUM_THREADS=4
export MKL_NUM_THREADS=4
export TORCH_HOME="$BASE/data/torch"
cd "$BASE/ST-Net"
mkdir -p "$BASE/output/pilot"
# One real-data epoch: all other patients train; BC23209 is held out.
python -m stnet run_spatial --gene --model densenet121 --pretrained \
    --window 224 --gene_n 250 --norm --lr 1e-6 --momentum 0.9 \
    --batch 32 --workers 4 --epochs 1 --gpu --average \
    --testpatients BC23209 --loglevel INFO \
    --logfile "$BASE/output/pilot/train.log" \
    --checkpoint "$BASE/output/pilot/checkpoints/epoch_" \
    --pred_root "$BASE/output/pilot/pred_"
python "$BASE/reproduction/evaluate.py" "$BASE/output/pilot/pred_1.npz"
