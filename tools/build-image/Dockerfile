FROM ubuntu:trusty
RUN apt-get -y update && apt-get install -y software-properties-common
RUN add-apt-repository --yes ppa:beineri/opt-qt571-trusty
RUN add-apt-repository --yes ppa:deadsnakes/ppa
RUN apt-get -y update && apt-get install -y gcc g++ make git qt57tools qt573d libgl1-mesa-dev libglu1-mesa-dev python3.5 clang vim
RUN mkdir code && cd code && git clone https://github.com/SpaceGroupUCL/depthmapX
