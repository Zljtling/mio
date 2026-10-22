"""Start and supervise one outer patient of the paper's primary experiment."""
import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import subprocess
import sys

BASE = Path(__file__).resolve().parent.parent
OUT = BASE / 'output/first_experiment'
STATUS = OUT / 'status.json'


def timestamp():
    return datetime.now(timezone.utc).isoformat()


def save(state):
    state['updated_at'] = timestamp()
    partial = STATUS.with_suffix('.tmp')
    partial.write_text(json.dumps(state, ensure_ascii=False, indent=2))
    partial.replace(STATUS)


def worker(simplified=False):
    state = dict(status='running', phase='fixed_split_training' if simplified else 'nested_cross_validation', patient='BC23209',
                 inner_folds=0 if simplified else 4, epochs_per_run=50, supervisor_pid=os.getpid(),
                 started_at=timestamp(), log=str(OUT / 'console.log'))
    save(state)
    try:
        command = (['bash', str(BASE / 'reproduction/run_simplified.sh')] if simplified else
                   ['bash', str(BASE / 'reproduction/run_full_cv.sh'), 'BC23209'])
        process = subprocess.Popen(command, cwd=BASE)
        state['training_pid'] = process.pid
        save(state)
        code = process.wait()
        if code:
            raise RuntimeError(f'Training exited with code {code}')
        selected = BASE / ('output/simplified/pred_50.npz' if simplified else 'output/full_cv/BC23209_cv.npz')
        assert selected.is_file(), f'Missing selected prediction: {selected}'
        state['phase'] = 'evaluation'
        state['selected_prediction'] = str(selected.resolve())
        save(state)
        note = ('Simplified experiment: fixed 22/1 patient split, held-out BC23209, '
                'fixed epoch 50, no inner cross-validation or test-driven epoch selection.' if simplified else
                'Primary experiment, one outer held-out patient BC23209; '
                'epoch selected using four-fold inner validation. '
                'Other 22 outer test patients have not been run.')
        subprocess.run([sys.executable, str(BASE / 'reproduction/evaluate.py'), str(selected),
                        '--note', note], check=True, cwd=BASE)
        state.update(status='completed', phase='finished', finished_at=timestamp(),
                     summary=str(selected.with_suffix('.summary.json')))
        save(state)
    except Exception as error:
        state.update(status='failed', error=str(error), finished_at=timestamp())
        save(state)
        raise


def main():
    global OUT, STATUS
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--worker', action='store_true')
    parser.add_argument('--simplified', action='store_true', help='Fixed split, 50 epochs, without inner CV')
    args = parser.parse_args()
    if args.simplified:
        OUT = BASE / 'output/simplified'
        STATUS = OUT / 'status.json'
    OUT.mkdir(parents=True, exist_ok=True)
    if args.worker:
        worker(args.simplified)
        return
    if STATUS.exists():
        previous = json.loads(STATUS.read_text())
        if previous['status'] == 'completed':
            print(json.dumps(previous, ensure_ascii=False, indent=2))
            return
        if previous['status'] == 'running':
            try:
                os.kill(previous['supervisor_pid'], 0)
            except ProcessLookupError:
                pass
            else:
                raise SystemExit('Experiment is already running; inspect status.json')
    with (OUT / 'console.log').open('a') as log:
        command = [sys.executable, str(Path(__file__).resolve()), '--worker']
        if args.simplified:
            command.append('--simplified')
        process = subprocess.Popen(command,
                                   cwd=BASE, stdin=subprocess.DEVNULL, stdout=log,
                                   stderr=subprocess.STDOUT, start_new_session=True)
    print(json.dumps(dict(supervisor_pid=process.pid, status_file=str(STATUS),
                          console_log=str(OUT / 'console.log')), ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
