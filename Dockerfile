# syntax=docker/dockerfile:1

# Neither gyc nor gyllir are built from source here: both are downloaded as prebuilt .deb
# release assets, published by the gymir (github.com/GNU-Ymir/gymir/releases) and Gyllir
# (github.com/GNU-Ymir/Gyllir/releases) repos respectively. GYC_RELEASE_TAG/GYC_ASSET and
# GYLLIR_RELEASE_TAG/GYLLIR_ASSET have no default here on purpose - YMIR_VERSION at the repo
# root is the single source of truth for these, and every CI workflow computes them from it.
# To build locally:
#   . ./YMIR_VERSION
#   GCC_MAJOR="${GCC_VERSION%%.*}"
#   docker build \
#     --build-arg GYC_RELEASE_TAG="$YMIR_BOOTSTRAP_VERSION" \
#     --build-arg GYC_ASSET="gyc-${GCC_MAJOR}_${YMIR_BOOTSTRAP_VERSION}_amd64.deb" \
#     --build-arg GYLLIR_RELEASE_TAG="$GYLLIR_VERSION" \
#     --build-arg GYLLIR_ASSET="gyllir_${GYLLIR_VERSION}_amd64.deb" \
#     .
ARG GYC_RELEASE_TAG
ARG GYC_ASSET
ARG GYLLIR_RELEASE_TAG
ARG GYLLIR_ASSET

FROM ubuntu:26.04 AS toolchain
ARG GYC_RELEASE_TAG
ARG GYC_ASSET
ARG GYLLIR_RELEASE_TAG
ARG GYLLIR_ASSET
ENV DEBIAN_FRONTEND=noninteractive

RUN test -n "$GYC_RELEASE_TAG" && test -n "$GYC_ASSET" \
    && test -n "$GYLLIR_RELEASE_TAG" && test -n "$GYLLIR_ASSET" || \
    (echo "GYC_RELEASE_TAG, GYC_ASSET, GYLLIR_RELEASE_TAG and GYLLIR_ASSET build-args are required - see YMIR_VERSION" >&2 && exit 1)

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates curl build-essential \
    && rm -rf /var/lib/apt/lists/*

# The gyc .deb depends on g++-<N>/gcc-<N>/libgc-dev/libdwarf-dev; `apt-get install ./file.deb`
# resolves those from the archive instead of a manual dpkg -i + apt --fix-broken dance.
RUN curl -fsSL -o /tmp/gyc.deb \
        "https://github.com/GNU-Ymir/gymir/releases/download/${GYC_RELEASE_TAG}/${GYC_ASSET}" \
    && curl -fsSL -o /tmp/gyllir.deb \
        "https://github.com/GNU-Ymir/Gyllir/releases/download/${GYLLIR_RELEASE_TAG}/${GYLLIR_ASSET}" \
    && apt-get update \
    && apt-get install -y --no-install-recommends /tmp/gyc.deb /tmp/gyllir.deb \
    && rm -f /tmp/gyc.deb /tmp/gyllir.deb \
    && rm -rf /var/lib/apt/lists/*

RUN gyc --version
RUN gyllir --version

FROM toolchain AS build
WORKDIR /midgard
COPY . .
RUN gyllir build

FROM build AS test
RUN ./midgard_tests -sf
