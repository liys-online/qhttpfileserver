# ---- Build Stage ----
FROM liyaosong/ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    cmake \
    ninja-build \
    g++ \
    git \
    python3 \
    python3-pip \
    python3-venv \
    libgl-dev \
    libxkbcommon-dev \
    libglib2.0-dev \
    && rm -rf /var/lib/apt/lists/*

# 使用 aqtinstall 安装 Qt 6.8
RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"
RUN pip install --no-cache-dir aqtinstall
RUN aqt install-qt linux desktop 6.8.3 linux_gcc_64 -O /opt/Qt -m qthttpserver qtwebsockets

ENV Qt6_DIR=/opt/Qt/6.8.3/gcc_64/lib/cmake/Qt6
ENV PATH="/opt/Qt/6.8.3/gcc_64/bin:$PATH"

WORKDIR /src
COPY . .

RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_GUI=OFF \
    && cmake --build build --parallel

# ---- Runtime Stage ----
FROM liyaosong/ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libgl1 \
    libxkbcommon0 \
    libglib2.0-0 \
    libkrb5-3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /src/build/httpserver /app/httpserver
COPY --from=builder /opt/Qt/6.8.3/gcc_64/lib/libQt6Core.so.6 /app/lib/
COPY --from=builder /opt/Qt/6.8.3/gcc_64/lib/libQt6Network.so.6 /app/lib/
COPY --from=builder /opt/Qt/6.8.3/gcc_64/lib/libQt6HttpServer.so.6 /app/lib/
COPY --from=builder /opt/Qt/6.8.3/gcc_64/lib/libQt6WebSockets.so.6 /app/lib/

ENV LD_LIBRARY_PATH=/app/lib

# 默认共享目录
RUN mkdir -p /data

EXPOSE 80

ENTRYPOINT ["/app/httpserver"]
CMD ["--path", "/data", "--port", "80"]
