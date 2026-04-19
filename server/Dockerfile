FROM ubuntu:24.04 AS BUILDER

RUN apt-get update && apt-get install -y \
    cmake g++ \
    libpqxx-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY CMakeLists.txt .
COPY src/ src/

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build

# final
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    libpqxx-dev \
    libcurl4-openssl-dev \
    netcat-openbsd \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/server .

CMD ["./server"]