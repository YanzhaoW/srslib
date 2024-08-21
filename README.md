# srslib - A library and command line client for SRS FEC & VMM3

## Introduction

### Why?

- dependency free
- simple, pure C (mostly C89)
- maintainable, 2k lines of code
- does not assume any specific (linux) environment
- strict separation between library and application

### Included

- control/monitoring for SRS FEC & VMM3a hybrids
- readout of data

### Not included:

- calibration routines
- graphical user interface
- sophisticated data transport


## Building

```
git clone -b dev https://github.com/YanzhaoW/srslib.git
cd srslib
git submodule update --init
cmake --preset default .
cmake --build ./build -- -j[nproc]
```

The executable programs are compiled in the build directory:

```
./build/apps/
```

## srscli - The control program

### Configure FEC + attached VMMs

```
./srscli
```

> **Note:**
> `srscli` assumes that the FEC has an IP address of 10.0.0.2.

### Start acquisition

```
./srscli --acq-on
```

### Stop acquisition

```
./srscli --acq-off
```

### Common options

```
-v/--verbose: Increase verbosity, can be given multiple times.
-h/--help:    Show help.
```

### Custom configuration

There is no support (yet) for reading a custom configuration from a config file. Instead, the user needs to modify the `fec_custom_config()` function in `apps/srscli/main.c`. Using configuration files is most probably the next feature on the roadmap.


## srsread - Reading data from a FEC

`srsread` is a simple readout loop for SRS/FEC.
Use the `-o/--output` option to store data to an output file.

```
./srsread -o /tmp/srs_fec_vmm3.dat
```

Data is stored **as is** from the FEC in binary format. No modifications are done.


## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.


## Acknowledgements

* A lot of information was used from the existing codebase of the VMM slow control software [vmmsc](https://gitlab.cern.ch/rd51-slow-control/vmmsc.git).


## Authors:

* **Bastian Löher** - *Initial work*
