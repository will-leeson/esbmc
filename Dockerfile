FROM ubuntu:20.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update
     
RUN apt-get install -y git

RUN apt-get install -qy nano

RUN apt-get install -y build-essential git gperf libgmp-dev cmake bison curl flex gcc-multilib linux-libc-dev libboost-all-dev libtinfo-dev ninja-build python3-setuptools unzip wget python3-pip openjdk-8-jre

RUN mkdir /home/esbmc-project/

RUN cd /home/esbmc-project/ 

RUN git clone https://github.com/will-leeson/esbmc.git /home/esbmc-project/esbmc

RUN wget https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.0/clang+llvm-11.0.0-x86_64-linux-gnu-ubuntu-20.04.tar.xz -P /home/esbmc-project/

RUN tar xJf /home/esbmc-project/clang+llvm-11.0.0-x86_64-linux-gnu-ubuntu-20.04.tar.xz && mv clang+llvm-11.0.0-x86_64-linux-gnu-ubuntu-20.04 /home/esbmc-project/clang11

RUN git clone --depth=1 --branch=3.2.1 https://github.com/boolector/boolector /home/esbmc-project/boolector && cd /home/esbmc-project/boolector && ./contrib/setup-lingeling.sh && ./contrib/setup-btor2tools.sh && ./configure.sh --prefix /home/esbmc-project/boolector-release && cd build && make -j4 && make install && cd .. && cd ..

RUN pip3 install toml && git clone https://github.com/CVC4/CVC4.git /home/esbmc-project/CVC4 && cd /home/esbmc-project/CVC4 && git reset --hard b826fc8ae95fc && ./contrib/get-antlr-3.4 && ./configure.sh --optimized --prefix=/home/esbmc-project/cvc4 --static --no-static-binary && cd build && make -j4 && make install && cd .. && cd ..

RUN wget http://mathsat.fbk.eu/download.php?file=mathsat-5.5.4-linux-x86_64.tar.gz -O /home/esbmc-project/mathsat.tar.gz && tar xf /home/esbmc-project/mathsat.tar.gz && mv mathsat-5.5.4-linux-x86_64 /home/esbmc-project/mathsat

RUN git clone https://github.com/SRI-CSL/yices2.git /home/esbmc-project/yices2 && cd /home/esbmc-project/yices2 && git checkout Yices-2.6.1 && autoreconf -fi && ./configure --prefix /home/esbmc-project/yices && make -j4 && make install && cd ..

RUN wget https://github.com/Z3Prover/z3/releases/download/z3-4.8.9/z3-4.8.9-x64-ubuntu-16.04.zip -P /home/esbmc-project/ && unzip /home/esbmc-project/z3-4.8.9-x64-ubuntu-16.04.zip -d /home/esbmc-project/ && mv /home/esbmc-project/z3-4.8.9-x64-ubuntu-16.04 /home/esbmc-project/z3

RUN git clone --depth=1 --branch=smtcomp-2021 https://github.com/bitwuzla/bitwuzla.git /home/esbmc-project/bitwuzla && cd /home/esbmc-project/bitwuzla && ./contrib/setup-lingeling.sh && ./contrib/setup-btor2tools.sh && ./contrib/setup-symfpu.sh && ./configure.sh --prefix /home/esbmc-project/bitwuzla-release && cd build && cmake -DONLY_LINGELING=ON ../ && make -j8 && make install && cd .. && cd ..

RUN wget https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.11.0%2Bcpu.zip -P /home/esbmc-project/ 

RUN unzip /home/esbmc-project/libtorch-cxx11-abi-shared-with-deps-1.11.0+cpu.zip -d /home/esbmc-project/

RUN git clone https://github.com/rusty1s/pytorch_sparse.git /home/esbmc-project/pytorch_sparse --recurse-submodules && cd /home/esbmc-project/pytorch_sparse && mkdir build && cd build && cmake -DCMAKE_PREFIX_PATH="/home/esbmc-project/libtorch/share/cmake/Torch/" .. && make -j4 install

RUN git clone https://github.com/rusty1s/pytorch_scatter.git /home/esbmc-project/pytorch_scatter && cd /home/esbmc-project/pytorch_scatter && mkdir build && cd build && cmake -DCMAKE_PREFIX_PATH="/home/esbmc-project/libtorch/share/cmake/Torch/" .. && make -j4 install

ENV CC=/home/esbmc-project/clang11/bin/clang
ENV CXX=/home/esbmc-project/clang11/bin/clang++


RUN cd /home/esbmc-project/esbmc && mkdir build && cd build && cmake .. -GNinja -DClang_DIR=/home/esbmc-project/clang11 -DLLVM_DIR=/home/esbmc-project/clang11 -DBoolector_DIR=/home/esbmc-project/boolector-release -DZ3_DIR=/home/esbmc-project/z3 -DENABLE_MATHSAT=ON -DMathsat_DIR=/home/esbmc-project/mathsat -DENABLE_YICES=On -DYices_DIR=/home/esbmc-project/yices -DCVC4_DIR=/home/esbmc-project/cvc4 -DBitwuzla_DIR=/home/esbmc-project/bitwuzla-release -DTorch_DIR=/home/esbmc-project/libtorch/share/cmake/Torch/ && ninja install

RUN git clone https://gitlab.com/sosy-lab/benchmarking/sv-benchmarks.git --branch=svcomp22 /home/esbmc-project/sv-benchmarks

ENV LD_LIBRARY_PATH=":/usr/local/lib/:/home/esbmc-project/libtorch/lib/:/home/esbmc-project/z3/bin/:/home/esbmc-project/mathsat/lib/:/home/esbmc-project/yices/lib/"

WORKDIR /home