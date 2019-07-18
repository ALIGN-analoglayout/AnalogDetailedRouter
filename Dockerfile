FROM ubuntu:18.04 as can_build

RUN apt-get update && apt-get install -yq curl g++ git build-essential lcov zlib1g-dev python3 python3-pip python3-venv && apt-get clean

FROM can_build as can_build_python

RUN \
    python3 -m venv general && \
    bash -c "source general/bin/activate && pip install --upgrade pip && pip install wheel pytest coverage pytest-cov"

FROM can_build_python as build_intel_detailed_router

COPY . /analog

RUN \
    cd /analog && \
    make clean && \
    make 

FROM can_build_python as intel_detailed_router

COPY --from=build_intel_detailed_router /analog/bin/amsr.exe /usr/bin