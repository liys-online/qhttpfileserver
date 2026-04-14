# ---- Build Stage ----
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    cmake \
    ninja-build \
    g++ \
    git \
    qt6-base-dev \
    qt6-httpserver-dev \
    libqt6httpserver6-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_GUI=OFF \
    && cmake --build build --parallel

# ---- Runtime Stage ----
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libqt6core6t64 \
    libqt6network6t64 \
    libqt6httpserver6t64 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /src/build/httpserver /app/httpserver

# 默认共享目录
RUN mkdir -p /data

EXPOSE 8080

ENTRYPOINT ["/app/httpserver"]
CMD ["--path", "/data", "--port", "8080"]
