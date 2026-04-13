FROM ubuntu:20.04

RUN apt-get update && apt-get install -y \
    g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY main.cpp .

RUN g++ -std=c++14 -O2 -D_REENTRANT -o msort main.cpp -lpthread

CMD ["./msort"]