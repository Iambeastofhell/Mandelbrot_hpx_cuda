# Results

- CPU: AMD Ryzen 7 7735HS with Radeon Graphics, 16 threads
- GPU: NVIDIA GeForce RTX 3050 6GB Laptop GPU
- Image size: 1920x1080

## tune

| View | Program | Options | Median (s) | Min (s) | Max (s) | Speedup | Mismatched | HPX threads |
|---|---|---|---|---|---|---|---|---|
| full | sequential |  | 1.3408 | 1.3402 | 1.3492 | 1.00 | 0 |  |
| full | hpx | --method pixel | 0.9507 | 0.9454 | 0.9874 | 1.41 | 0 | 2.07366 |
| full | hpx | --method row | 0.0973 | 0.0968 | 0.0989 | 13.78 | 0 | 1137 |
| full | hpx | --method tile --tile 16 | 0.1051 | 0.1047 | 0.1087 | 12.76 | 0 | 8217 |
| full | hpx | --method tile --tile 32 | 0.1003 | 0.1000 | 0.1010 | 13.37 | 0 | 2097 |
| full | hpx | --method tile --tile 64 | 0.1004 | 0.0998 | 0.1007 | 13.35 | 0 | 597 |
| full | hpx | --method tile --tile 128 | 0.1206 | 0.1192 | 0.1211 | 11.12 | 0 | 207 |
| full | hpx | --method tile --tile 256 | 0.1665 | 0.1630 | 0.1723 | 8.05 | 0 | 105 |
| full | hpx | --method loop --chunk 0 | 0.1167 | 0.1159 | 0.1180 | 11.49 | 0 | 73 |
| full | hpx | --method loop --chunk 64 | 0.1019 | 0.1017 | 0.1027 | 13.16 | 0 | 73 |
| full | hpx | --method loop --chunk 512 | 0.1022 | 0.1019 | 0.1035 | 13.12 | 0 | 73 |
| full | hpx | --method loop --chunk 4096 | 0.1027 | 0.1016 | 0.1043 | 13.05 | 0 | 73 |
| full | hpx | --method loop --chunk 32768 | 0.1168 | 0.1164 | 0.1205 | 11.48 | 0 | 73 |
| full | hpx | --method border --tile 32 --min-tile 8 | 0.0377 | 0.0377 | 0.0396 | 35.54 | 1 | 19912 |
| full | hpx | --method border --tile 32 --min-tile 16 | 0.0365 | 0.0353 | 0.0394 | 36.71 | 0 | 6792 |
| full | hpx | --method border --tile 32 --min-tile 32 | 0.0365 | 0.0361 | 0.0373 | 36.73 | 0 | 6547 |
| full | hpx | --method border --tile 64 --min-tile 8 | 0.0359 | 0.0350 | 0.0361 | 37.30 | 0 | 19412 |
| full | hpx | --method border --tile 64 --min-tile 16 | 0.0340 | 0.0337 | 0.0360 | 39.49 | 0 | 6647 |
| full | hpx | --method border --tile 64 --min-tile 32 | 0.0377 | 0.0365 | 0.0400 | 35.59 | 0 | 2067 |
| full | hpx | --method border --tile 128 --min-tile 8 | 0.0349 | 0.0346 | 0.0364 | 38.42 | 1 | 19217 |
| full | hpx | --method border --tile 128 --min-tile 16 | 0.0321 | 0.0319 | 0.0326 | 41.81 | 0 | 6647 |
| full | hpx | --method border --tile 128 --min-tile 32 | 0.0377 | 0.0368 | 0.0413 | 35.61 | 0 | 2147 |
| full | hpx | --method border --tile 256 --min-tile 8 | 0.0364 | 0.0357 | 0.0410 | 36.87 | 1 | 18775 |
| full | hpx | --method border --tile 256 --min-tile 16 | 0.0333 | 0.0325 | 0.0369 | 40.21 | 0 | 6490 |
| full | hpx | --method border --tile 256 --min-tile 32 | 0.0364 | 0.0358 | 0.0388 | 36.80 | 0 | 2125 |

## views

| View | Program | Options | Median (s) | Min (s) | Max (s) | Speedup | Mismatched | HPX threads |
|---|---|---|---|---|---|---|---|---|
| full | sequential |  | 1.3528 | 1.3484 | 1.3635 | 1.00 | 0 |  |
| full | hpx | --method row | 0.0980 | 0.0978 | 0.0986 | 13.80 | 0 | 1137 |
| full | hpx | --method tile --tile 64 | 0.1015 | 0.1001 | 0.1091 | 13.32 | 0 | 597 |
| full | hpx | --method loop --chunk 0 | 0.1161 | 0.1157 | 0.1167 | 11.65 | 0 | 73 |
| full | hpx | --method border --tile 64 --min-tile 16 | 0.0341 | 0.0337 | 0.0348 | 39.70 | 0 | 6647 |
| full | cuda |  | 0.1051 | 0.1040 | 0.1736 | 12.87 | 0 |  |
| interior | sequential |  | 8.1373 | 8.1316 | 8.1921 | 1.00 | 0 |  |
| interior | hpx | --method row | 0.6019 | 0.5949 | 0.6092 | 13.52 | 0 | 1137 |
| interior | hpx | --method tile --tile 64 | 0.5909 | 0.5856 | 0.6101 | 13.77 | 0 | 597 |
| interior | hpx | --method loop --chunk 0 | 0.6061 | 0.6028 | 0.6156 | 13.43 | 0 | 73 |
| interior | hpx | --method border --tile 64 --min-tile 16 | 0.0652 | 0.0644 | 0.0685 | 124.75 | 0 | 2087 |
| interior | cuda |  | 0.5868 | 0.5863 | 0.6536 | 13.87 | 0 |  |
| edges | sequential |  | 0.4024 | 0.4007 | 0.4032 | 1.00 | 0 |  |
| edges | hpx | --method row | 0.0342 | 0.0309 | 0.0361 | 11.76 | 0 | 1137 |
| edges | hpx | --method tile --tile 64 | 0.0321 | 0.0315 | 0.0324 | 12.54 | 0 | 615 |
| edges | hpx | --method loop --chunk 0 | 0.0332 | 0.0323 | 0.0348 | 12.12 | 0 | 73 |
| edges | hpx | --method border --tile 64 --min-tile 16 | 0.0397 | 0.0394 | 0.0419 | 10.14 | 0 | 10135 |
| edges | cuda |  | 0.0791 | 0.0777 | 0.1079 | 5.08 | 0 |  |

## threads

| View | Program | Options | Median (s) | Min (s) | Max (s) | Speedup | Mismatched | HPX threads |
|---|---|---|---|---|---|---|---|---|
| full | sequential |  | 1.3470 | 1.3433 | 1.3495 | 1.00 | 0 |  |
| full | hpx | --method row --threads 1 | 1.3641 | 1.3614 | 1.3667 | 0.99 | 0 | 1092 |
| full | hpx | --method row --threads 2 | 0.6928 | 0.6915 | 0.6937 | 1.94 | 0 | 1095 |
| full | hpx | --method row --threads 4 | 0.3521 | 0.3513 | 0.3525 | 3.83 | 0 | 1101 |
| full | hpx | --method row --threads 8 | 0.1799 | 0.1788 | 0.1807 | 7.49 | 0 | 1113 |
| full | hpx | --method row --threads 16 | 0.0971 | 0.0967 | 0.0997 | 13.87 | 0 | 1137 |
| full | hpx | --method border --threads 1 | 0.3534 | 0.3509 | 0.3536 | 3.81 | 0 | 6602 |
| full | hpx | --method border --threads 2 | 0.1793 | 0.1789 | 0.1814 | 7.51 | 0 | 6605 |
| full | hpx | --method border --threads 4 | 0.0928 | 0.0923 | 0.0932 | 14.52 | 0 | 6611 |
| full | hpx | --method border --threads 8 | 0.0493 | 0.0489 | 0.0498 | 27.35 | 0 | 6623 |
| full | hpx | --method border --threads 16 | 0.0338 | 0.0336 | 0.0341 | 39.86 | 0 | 6647 |

## explorer

| View | Program | Options | Median (s) | Min (s) | Max (s) | Mismatched | Tiles |
|---|---|---|---|---|---|---|---|
| full | explorer | --pans 32 | 1.1530 | 1.1427 | 1.1658 | 0 | 630 |
| full | explorer | --pans 32 --no-reuse | 1.8840 | 1.8689 | 1.9071 | 0 | 17190 |
| edges | explorer | --pans 32 | 1.1810 | 1.1716 | 1.1963 | 0 | 638 |
| edges | explorer | --pans 32 --no-reuse | 2.1042 | 2.0892 | 2.1322 | 0 | 17856 |
