# Compiling and Flashing Firmware

Quail Maker firmware requires that the [Particle CLI](https://docs.particle.io/tutorials/developer-tools/cli/) is installed. Using the Particle CLI, the firmware can be compiled with:

```
particle compile photon
```

And flashed by using the flash command with the resulting binary:

```
particle flash quail_maker photon_firmware_xxxxxxxxxxxxx.bin
```