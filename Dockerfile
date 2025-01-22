FROM debian:stable-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    ca-certificates \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    wget \
    libmicrohttpd-dev \
    libjansson-dev \
    libb64-dev \
    uuid-dev \
    inotify-tools \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*


WORKDIR /app

COPY . /app

RUN mkdir -p /app/build && \
    cd /app/build && \
    cmake .. && \
    make

WORKDIR /app/build

EXPOSE 8080

# Step 9: Command to run the application
CMD ["sh", "-c", "./katz & inotifywait -m -e modify -e create -e delete --format '%w%f' /app/src/routes | while read FILE; do \
     echo \"[DEBUG] Detected change in file: $FILE\"; \
     ls -l --time-style=full-iso /app/src/routes; \
     MAKE_OUTPUT=$(make -C /app/build 2>&1); \
     if [ $? -eq 0 ]; then \
         echo \"[INFO] make completed successfully\"; \
     else \
         echo \"[ERROR] make failed with exit code $?\"; \
         echo \"$MAKE_OUTPUT\"; \
     fi; \      
     done"]