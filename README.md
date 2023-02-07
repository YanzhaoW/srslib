# srscli - A command line client + library for SRS FEC & VMM3


## Building

```
git clone https://github.com/bl0x/srscli
cd srscli
make
```

The executable programs are compiled in the build directory:

```
./build_*/bin/
```

The shared library object is

```
./build_*/libsrs.so
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
