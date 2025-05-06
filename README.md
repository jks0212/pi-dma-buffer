# pi-dma-buffer

## Overview

`pi-dma-buffer` is a Linux kernel module for the Raspberry Pi that allocates and uses uncached memory. It reserves a fixed-size buffer of 1 MB by default, which can be modified by changing the `BUFFER_SIZE` in the source code.

## Installation

1. Build the module:
   ```bash
   make

2. Insert the module into the kernel:
   ```bash
   sudo insmod dma_buffer.ko
