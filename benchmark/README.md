# Benchmarks

```bash
make all explorer     # also run make cuda on a machine with an NVIDIA GPU
python3 benchmark/run.py
```

This writes `results.csv`, with one row per run, and `results.md`, with one table per suite. Only generation is timed.

| Suite | Compares |
|---|---|
| `tune` | The HPX methods with several tile and chunk sizes, against the sequential program |
| `views` | Versions 1, 2, 3 and 5 on three views |
| `threads` | The row and border methods with one thread up to one per CPU |
| `explorer` | Versions 4a and 4b over 32 pans |

The views are the whole set, a view that is mostly inside the set, and a view full of fine edges. The CUDA program is included if it runs.

After running `tune`, set the best sizes in `METHODS` at the top of `run.py` before running `views`.

Options: `--suite NAME` (repeatable), `--runs N`, `--size W H`, `--build DIR` and `--out DIR`.

## Columns

| Column | Meaning |
|---|---|
| Speedup | Sequential median divided by the median of this case |
| Mismatched | Pixels whose count differs from the sequential program |
| HPX threads | HPX threads run, including a few of the runtime's own |
| Idle (%) | Share of time the HPX workers had no work |
| Tiles | Tiles the explorer computed during the pans |

The idle rate needs an HPX built with `HPX_WITH_THREAD_IDLE_RATES=ON` and is left out otherwise.

Close other programs before a run. Times from different machines cannot be compared.
