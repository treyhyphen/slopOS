FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-24.04

ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="3.22.2"

# Optionally install the cmake for vcpkg
COPY ./reinstall-cmake.sh /tmp/

RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
        chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh

# [Optional] Uncomment this section to install additional vcpkg ports.
# RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

# Install packages for slopOS development (bootable ISO creation and testing)
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
    # Assembler for boot.s
    nasm \
    # GRUB bootloader tools for creating bootable ISOs
    grub-pc-bin \
    grub-common \
    # ISO creation tools
    xorriso \
    mtools \
    # QEMU for testing bootable ISOs
    qemu-system-x86 \
    # Additional utilities
    binutils \
    && apt-get clean -y && rm -rf /var/lib/apt/lists/*
