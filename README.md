# pi-dma-buffer

## Overview

`pi-dma-buffer` is a Linux kernel module for the Raspberry Pi that allocates and uses uncached memory. It reserves a fixed-size buffer of 1 MB by default, which can be modified by changing the `BUFFER_SIZE` in the source code.

## Installation

1. Build the module:
   ```bash
   make
   ```

2. Insert the module into the kernel:
   ```bash
   sudo insmod dma_buffer.ko
   ```

## How to use

1. Open device:
   ```c
   int fd = open("/dev/dma_buffer", O_RDWR | O_SYNC);
   if (fd < 0) {
      perror("open error");
   }
   ```

2. Mapping on userspace:
   ```c
   void *virt_addr = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   if (virt_addr == MAP_FAILED) {
      perror("mmap error");
   } else{
      printf("virt_addr = %p\n", virt_addr);
   }
   ```

3. Get physical address:
   ```c
   #define DMA_BUF_IOCTL_GET_PHYS_ADDR _IOR(0xF0, 1, uint32_t)
   
   uint32_t phys_addr;
   int ret = ioctl(fd, DMA_BUF_IOCTL_GET_PHYS_ADDR, &phys_addr);
   if (ret < 0) {
      perror("ioctl error");
   } else{
      printf("phys_addr = 0x%08x\n", phys_addr);
   }
   ```
    
