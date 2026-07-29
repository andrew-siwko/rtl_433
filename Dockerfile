FROM debian:bookworm-slim

RUN apt-get update -q -y && apt-get install -q -y --no-install-recommends \
        cmake build-essential pkg-config \
        libusb-1.0-0-dev librtlsdr-dev libsoapysdr-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build

ENTRYPOINT ["/src/build/src/rtl_433"]
