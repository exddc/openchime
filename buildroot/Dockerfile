# Buildroot build environment for Open Chime
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    wget \
    cpio \
    unzip \
    rsync \
    bc \
    libncurses-dev \
    flex \
    bison \
    libssl-dev \
    libelf-dev \
    python3 \
    python3-pip \
    python3-setuptools \
    file \
    locales \
    sudo \
    curl \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY bun-version /tmp/bun-version
RUN set -eux; \
    BUN_VERSION="$(tr -d '[:space:]' < /tmp/bun-version)"; \
    arch="$(uname -m)"; \
    case "$arch" in \
      x86_64) bun_arch=x64; bun_sha=951ee2aee855f08595aeec6225226a298d3fea83a3dcd6465c09cbccdf7e848f ;; \
      aarch64) bun_arch=aarch64; bun_sha=a27ffb63a8310375836e0d6f668ae17fa8d8d18b88c37c821c65331973a19a3b ;; \
      *) echo "unsupported arch: $arch" >&2; exit 1 ;; \
    esac; \
    curl -fsSL -o /tmp/bun.zip \
      "https://github.com/oven-sh/bun/releases/download/bun-v${BUN_VERSION}/bun-linux-${bun_arch}.zip"; \
    echo "${bun_sha}  /tmp/bun.zip" | sha256sum -c; \
    unzip -q /tmp/bun.zip -d /tmp/bun-extract; \
    install -m0755 "/tmp/bun-extract/bun-linux-${bun_arch}/bun" /usr/local/bin/bun; \
    rm -rf /tmp/bun.zip /tmp/bun-extract; \
    /usr/local/bin/bun --version

RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8
RUN useradd -m -s /bin/bash -u 1000 builder && \
    echo "builder ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

WORKDIR /home/builder
USER builder
