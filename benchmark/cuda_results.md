# CUDA results

- GPU: NVIDIA L40 (one cluster node)
- Driver: 530.30.02
- CUDA: 11.3
- PTX target: `compute_80`

Raw output: [cuda_cluster.txt](cuda_cluster.txt)

CUDA and sequential outputs were identical in all four cases tested.

| Case (1001x777) | CUDA kernel (s) | CUDA total (s) | Sequential (s) | Speedup |
|---|---|---|---|---|
| zoomed view, gradient | 0.0011 | 0.0023 | 0.1597 | 70x |
| grayscale | 0.0005 | 0.0016 | 0.0411 | 25x |
| 2000 iterations | 0.0069 | 0.0080 | 0.6598 | 83x |

CUDA total includes device memory allocation and the copy back to the host. Speedup is sequential time over CUDA total. Each time is from a single run.
