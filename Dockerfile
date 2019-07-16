FROM ubuntu:18.04 as can_build

RUN apt-get update && apt-get install -yq curl g++ git build-essential lcov zlib1g-dev && apt-get clean

FROM can_build as build_intel_detailed_router

COPY . /analog

RUN \
    cd /analog && \
    make clean && \
    make 

FROM can_build as intel_detailed_router

COPY --from=build_intel_detailed_router /analog/bin/amsr.exe /usr/bin