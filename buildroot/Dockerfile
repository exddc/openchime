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

ARG BUN_VERSION=1.3.14
RUN curl -fsSL https://bun.sh/install | BUN_INSTALL=/usr/local bash -s "bun-v${BUN_VERSION}" \
    && /usr/local/bin/bun --version

RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8
RUN useradd -m -s /bin/bash -u 1000 builder && \
    echo "builder ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

WORKDIR /home/builder
USER builder
