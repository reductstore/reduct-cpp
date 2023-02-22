FROM ubuntu:22.04 AS builder
RUN apt update && apt install -y build-essential cmake python3-pip

RUN pip3 install conan==1.58.0
RUN conan profile new default --detect

WORKDIR /src

COPY . .

WORKDIR /build

ARG BUILD_TYPE=Release
RUN cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE}  -DREDUCT_CPP_ENABLE_EXAMPLES=ON -DREDUCT_CPP_ENABLE_TESTS=ON /src
RUN make -j4

FROM ubuntu:22.04

COPY --from=builder /build/bin/ /usr/local/bin/
