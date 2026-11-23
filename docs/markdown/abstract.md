---
title: "Interactive Mandelbrot Set Explorer using HPX and CUDA"
colorlinks: true
linkcolor: blue
urlcolor: blue
toccolor: blue
citecolor: blue
geometry: top=1.5cm, left=2.5cm, right=2.5cm, bottom=2cm
---

## Team Members

1. Rajdeep (112301045)
2. Zeeshan Mohammed Rangrej (112301040)
3. Vishesh Srivastava (112301037)
4. Rajat Bhootra (112301044)

\

## Abstract

This project aims to develop an application to interactively visualize the Mandelbrot set while exploring parallel and heterogeneous computing.

The application will support real-time navigation through the fractal by means
of panning and zooming, along with configurable iteration limits and color
schemes.

The computational backend will be explored through various implementations:

1. Sequential CPU execution
2. Task-based parallelism using HPX
3. GPU acceleration using CUDA
4. HPX-managed GPU execution

These implementations will provide different approaches to distributing and
executing the computational workloads, allowing us to investigate their
strengths and trade-offs. The performance across the implementations will be
compared on the basis of various factors such as execution time, scalability,
and load balancing.

\

## References

1. [Mandelbrot set (Wikipedia)](https://en.wikipedia.org/wiki/Mandelbrot_set)
2. [HPX (High Performance ParalleX)](https://hpx.stellar-group.org/)
3. [CUDA](https://developer.nvidia.com/cuda)

