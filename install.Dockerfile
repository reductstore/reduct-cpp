FROM gcc:11.2 AS builder
RUN apt update && apt install -y cmake

WORKDIR /src

COPY src src
COPY CMakeLists.txt .

WORKDIR /build

ARG BUILD_TYPE=Release
RUN cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DREDUCT_CPP_ENABLE_TESTS=OFF -DREDUCT_CPP_ENABLE_EXAMPLES=OFF /src
RUN make -j4
RUN make install

FROM ubuntu:21.10

COPY --from=builder /build/bin/ /usr/local/bin/