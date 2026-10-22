"""Wait for the existing training job, then render and audit epoch-50 figures."""
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import subprocess
import sys
import time

BASE = Path(__file__).resolve().parents[2]
OUT = BASE / 'output/figures'
STATUS = OUT / 'render-status.json'


def save(state):
    state['updated_at'] = datetime.now(timezone.utc).isoformat()
    partial = STATUS.with_suffix('.tmp')
    partial.write_text(json.dumps(state, ensure_ascii=False, indent=2))
    partial.replace(STATUS)


def worker():
    state = dict(status='waiting_for_training', pid=os.getpid(), target_epoch=50,
                 output='output/figures/final_epoch_050', visual_review='pending_after_final_render')
    save(state)
    try:
        while True:
            training = json.loads((BASE / 'output/simplified/status.json').read_text())
            if training['status'] == 'completed':
                break
            if training['status'] != 'running':
                raise RuntimeError('Training has stopped or failed; final figures were not generated')
            try:
                os.kill(training['supervisor_pid'], 0)
            except ProcessLookupError:
                raise RuntimeError('Training supervisor is no longer running')
            time.sleep(30)
        state['status'] = 'rendering'
        save(state)
        subprocess.run([sys.executable, str(Path(__file__).with_name('plot_results.py')),
                        '--prediction', str(BASE / 'output/simplified/pred_50.npz'), '--final', '--png-only'],
                       cwd=BASE, check=True)
        state.update(status='completed', automated_qa='passed',
                     visual_review='Final figures require human visual review; preview was inspected separately')
        save(state)
    except Exception as error:
        state.update(status='failed', error=str(error))
        save(state)
        raise


if __name__ == '__main__':
    OUT.mkdir(parents=True, exist_ok=True)
    if '--worker' in sys.argv:
        worker()
    else:
        if STATUS.exists():
            previous = json.loads(STATUS.read_text())
            if previous['status'] == 'completed':
                raise SystemExit('Final figures are already complete')
            if previous['status'] in ['waiting_for_training', 'rendering']:
                try:
                    os.kill(previous['pid'], 0)
                except ProcessLookupError:
                    pass
                else:
                    raise SystemExit('Figure watcher is already running')
        with (OUT / 'final-render.log').open('a') as log:
            process = subprocess.Popen([sys.executable, str(Path(__file__).resolve()), '--worker'],
                                       cwd=BASE, stdin=subprocess.DEVNULL, stdout=log,
                                       stderr=subprocess.STDOUT, start_new_session=True)
        print(json.dumps(dict(watcher_pid=process.pid, status=str(STATUS)), ensure_ascii=False))
