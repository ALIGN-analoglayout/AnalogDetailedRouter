### AnalogDetailedRouter
[![CircleCI](https://circleci.com/gh/ALIGN-analoglayout/AnalogDetailedRouter.svg?style=svg)](https://circleci.com/gh/ALIGN-analoglayout/AnalogDetailedRouter)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/a134e059825f4e61875d9d105d3f2325)](https://www.codacy.com/app/ALIGN-analoglayout/AnalogDetailedRouter?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=ALIGN-analoglayout/AnalogDetailedRouter&amp;utm_campaign=Badge_Grade)

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
TO test:
```bash
(cd tests/vga && ../../bin/amsr.exe -file INPUT/ctrl.txt)
```
or
```bash
docker run --name cp --mount source=rVol,target=/run ubuntu
docker cp tests/vga/INPUT cp:/run/INPUT
docker cp tests/vga/DR_COLLATERAL cp:/run/DR_COLLATERAL
docker cp tests/vga/out-gold cp:/run/out-gold
docker rm cp
docker run -it --mount source=rVol,target=/run intel_detailed_router bash -c "cd run && amsr.exe -file INPUT/ctrl.txt && diff out out-gold"
```
### License
BSD 3 clause

### Release Notes

First open source release

Current static code analysis issues:
* Minor memory leaks
* Divide by zero possible on incorrect inputs
