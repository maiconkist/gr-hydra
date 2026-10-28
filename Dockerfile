FROM ubuntu:24.04

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        cppzmq-dev \
        gnuradio \
        gnuradio-dev \
        libboost-filesystem-dev \
        libboost-log-dev \
        libboost-program-options-dev \
        libboost-system-dev \
        libfftw3-dev \
        libopencv-dev \
        libuhd-dev \
        pkg-config \
        pybind11-dev \
        python3-dev \
        python3-lxml \
        python3-yaml \
        python3-zmq \
        uhd-host \
    && rm -rf /var/lib/apt/lists

WORKDIR /opt/hydra
COPY . .

ARG BUILD_JOBS=2

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    && cmake --build build --parallel "${BUILD_JOBS}" \
    && cmake --install build \
    && ldconfig

RUN cmake -S grc_blocks -B grc_blocks/build -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    && cmake --build grc_blocks/build --parallel "${BUILD_JOBS}" \
    && cmake --install grc_blocks/build \
    && ldconfig

RUN cmake -S tests -B tests/build \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DHYDRA_LIBRARY=/opt/hydra/build/lib/libhydra.so \
    && cmake --build tests/build --parallel "${BUILD_JOBS}" \
    && ctest --test-dir tests/build --output-on-failure --timeout 60 \
    && python3 -m unittest discover -s tests -p 'qa_*.py' -v

RUN useradd --create-home hydra \
    && chown -R hydra:hydra /opt/hydra/tests/build

USER hydra
WORKDIR /home/hydra
CMD ["/bin/sh", "-c", "ctest --test-dir /opt/hydra/tests/build --output-on-failure --timeout 60 && exec python3 -m unittest discover -s /opt/hydra/tests -p 'qa_*.py' -v"]
