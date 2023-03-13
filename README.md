# Purpose
The purpose of this project is to provide the host code for a hardware accelerator. The accelerator is implemented in the FPGA fabric on a Cyclone V SoC. Particularly, this is intended to run on the DE1-SoC board. Currently this project performs some benchmarks on interface bandwidth.

# Features
- WIP
- SDRAM bandwidth in excess of 2.5 MBps
# Future Features
- TensorFlowLite compatability
# How To Use

## SoC Board
1. Install [udmabuf](https://github.com/ikwzm/udmabuf) on your SoC board 
2. Configure your SoC board to initialize the SDRAM controller (various options)
   - For the DE1-SoC you need to copy your .rbf file to `/media/fat_partition/soc_system.rbf`. On startup the boot loader (U-Boot iirc) will automatically configure the SDRAM controller according to the FPGA image (number of activated ports, AXI vs Avalon MM). `MSEL=0b00000` so the image is loaded on HPS reset (this works for me but there are other issues I have, maybe I have some wonky configuration that I haven't looked at yet).
## Development Environment
1. Install `libbsd-dev` for `bsd/string.h` and `gcc-arm*` for `arm-linux-gnueabihf-gcc`.
```
sudo apt install -y libbsd-dev gcc-arm*
```
1. Enter your SoC EDS environment shell.
2. Run `make executable`, to build the project executable. This can be moved to the SoC to be
   to be run.