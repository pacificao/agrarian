ARG UBUNTU_VERSION=26.04
FROM ubuntu:${UBUNTU_VERSION}

RUN apt-get -o Acquire::Retries=3 update \
    && apt-get -o Acquire::Retries=3 install -y --no-install-recommends \
        python3 xvfb xauth ca-certificates fonts-dejavu-core ocl-icd-libopencl1 \
        libegl1 libgl1 libfontconfig1 libfreetype6 libharfbuzz0b libcurl4t64 \
        libglib2.0-0t64 libxcb-cursor0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 \
        libxcb-randr0 libxcb-render-util0 libxcb-shape0 libxcb-sync1 libxcb-xfixes0 \
        libxcb-xinerama0 libxcb-xkb1 libxkbcommon-x11-0 libx11-xcb1 libxi6 libxrender1 \
    && apt-get clean

COPY test/functional/desktop_package_smoke.py test/functional/first_party_pool_guard.py test/functional/gpu_pool_smoke.py /qa/
COPY test/containers/ubuntu-wallet-smoke.sh /qa/

ENTRYPOINT ["/bin/bash", "/qa/ubuntu-wallet-smoke.sh"]
