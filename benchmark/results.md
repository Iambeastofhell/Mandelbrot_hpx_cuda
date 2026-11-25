CPU: AMD Ryzen 5 3500U with Radeon Vega Mobile Gfx

### resolution

| Size | Max iter | Color | View | Threads | Grain | Sequential (s) | HPX min (s) | HPX median (s) | HPX max (s) | Speedup | Efficiency |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1024x1024 | 500 | gradient | full | 8 | row | 0.3182 | 0.0530 | 0.0545 | 0.0590 | 5.84x | 73% |
| 1920x1080 | 500 | gradient | full | 8 | row | 0.6665 | 0.1021 | 0.1133 | 0.1210 | 5.88x | 74% |
| 256x256 | 500 | gradient | full | 8 | row | 0.0278 | 0.0057 | 0.0060 | 0.0105 | 4.61x | 58% |
| 3840x2160 | 500 | gradient | full | 8 | row | 2.5004 | 0.5059 | 0.5099 | 0.5235 | 4.90x | 61% |
| 512x512 | 500 | gradient | full | 8 | row | 0.0900 | 0.0150 | 0.0153 | 0.0164 | 5.89x | 74% |

### threads

| Size | Max iter | Color | View | Threads | Grain | Sequential (s) | HPX min (s) | HPX median (s) | HPX max (s) | Speedup | Efficiency |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1920x1080 | 500 | gradient | full | 1 | row | 0.6345 | 0.6045 | 0.6096 | 0.6376 | 1.04x | 104% |
| 1920x1080 | 500 | gradient | full | 2 | row | 0.6345 | 0.3307 | 0.3409 | 0.3485 | 1.86x | 93% |
| 1920x1080 | 500 | gradient | full | 4 | row | 0.6345 | 0.1727 | 0.1841 | 0.1898 | 3.45x | 86% |
| 1920x1080 | 500 | gradient | full | 8 | row | 0.6345 | 0.1013 | 0.1042 | 0.1077 | 6.09x | 76% |

### iterations

| Size | Max iter | Color | View | Threads | Grain | Sequential (s) | HPX min (s) | HPX median (s) | HPX max (s) | Speedup | Efficiency |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1024x1024 | 100 | gradient | full | 8 | row | 0.0875 | 0.0197 | 0.0219 | 0.0292 | 4.01x | 50% |
| 1024x1024 | 1000 | gradient | full | 8 | row | 0.5996 | 0.0969 | 0.0974 | 0.0996 | 6.15x | 77% |
| 1024x1024 | 2000 | gradient | full | 8 | row | 1.1821 | 0.1819 | 0.1869 | 0.1991 | 6.32x | 79% |
| 1024x1024 | 500 | gradient | full | 8 | row | 0.3271 | 0.0525 | 0.0540 | 0.0545 | 6.06x | 76% |

### color

| Size | Max iter | Color | View | Threads | Grain | Sequential (s) | HPX min (s) | HPX median (s) | HPX max (s) | Speedup | Efficiency |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1024x1024 | 500 | blackwhite | full | 8 | row | 0.3425 | 0.0546 | 0.0624 | 0.1045 | 5.49x | 69% |
| 1024x1024 | 500 | gradient | full | 8 | row | 0.3297 | 0.0545 | 0.0594 | 0.0682 | 5.55x | 69% |
| 1024x1024 | 500 | grayscale | full | 8 | row | 0.3168 | 0.0526 | 0.0543 | 0.0595 | 5.83x | 73% |

### grain

| Size | Max iter | Color | View | Threads | Grain | Sequential (s) | HPX min (s) | HPX median (s) | HPX max (s) | Speedup | Efficiency |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1024x1024 | 500 | gradient | full | 8 | 1 | 0.3275 | 1.0597 | 1.0806 | 1.2037 | 0.30x | 4% |
| 1024x1024 | 500 | gradient | full | 8 | 16 | 0.3275 | 0.0824 | 0.1057 | 0.1407 | 3.10x | 39% |
| 1024x1024 | 500 | gradient | full | 8 | 256 | 0.3275 | 0.0607 | 0.0622 | 0.0633 | 5.26x | 66% |
| 1024x1024 | 500 | gradient | full | 8 | 1024 | 0.3275 | 0.0542 | 0.0564 | 0.0608 | 5.81x | 73% |
| 1024x1024 | 500 | gradient | full | 8 | 4096 | 0.3275 | 0.0525 | 0.0529 | 0.0540 | 6.20x | 77% |
| 1024x1024 | 500 | gradient | full | 8 | 16384 | 0.3275 | 0.0575 | 0.0595 | 0.0662 | 5.51x | 69% |
| 1024x1024 | 500 | gradient | full | 8 | 65536 | 0.3275 | 0.0809 | 0.0840 | 0.0948 | 3.90x | 49% |
| 1024x1024 | 500 | gradient | full | 8 | 131072 | 0.3275 | 0.1162 | 0.1979 | 0.2170 | 1.66x | 21% |

### view

| Size | Max iter | Color | View | Threads | Grain | Sequential (s) | HPX min (s) | HPX median (s) | HPX max (s) | Speedup | Efficiency |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1024x1024 | 500 | gradient | full | 8 | row | 0.3172 | 0.0518 | 0.0619 | 0.0713 | 5.13x | 64% |
| 1024x1024 | 500 | gradient | zoom | 8 | row | 1.3280 | 0.2112 | 0.2210 | 0.2392 | 6.01x | 75% |

