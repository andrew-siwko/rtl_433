FROM debian:bookworm-slim

RUN apt-get update -q -y && apt-get install -q -y --no-install-recommends \
        libusb-1.0-0 librtlsdr0 libsoapysdr0.8 \
    && rm -rf /var/lib/apt/lists/*

COPY rtl_433 /usr/local/bin/rtl_433

ENTRYPOINT ["/usr/local/bin/rtl_433"]
