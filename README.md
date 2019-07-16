### AnalogDetailedRouter

This repository contains the Intel's Analog Detailed Router.

To build:
```bash
sudo apt-get update
sudo apt-get install g++ build-essential zlib1g-dev
make clean
make
```
or
```bash
docker build -t intel_detailed_router .
```

### License
BSD 3 clause

### Release Notes

First open source release

Current static code analysis issues:
* Minor memory leaks
* Divide by zero possible on incorrect inputs
