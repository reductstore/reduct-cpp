FROM gcc:11.2 AS builder
RUN apt update && apt install -y cmake python3-pip

RUN pip3 install conan

WORKDIR /src

COPY conanfile.txt .

COPY src src
COPY tests tests
COPY CMakeLists.txt .

WORKDIR /build

ARG BUILD_TYPE=Release
RUN cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} /src
RUN make -j4

FROM ubuntu:21.10

COPY --from=builder /build/bin/ /usr/local/bin/