FROM ubuntu:24.04 AS ubuntu2404_common_deps
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && \
    apt install -y \
    git \
    python3 \
    build-essential \
    xz-utils \
    wget \
    clang-format \
    unzip

RUN wget -O - https://raw.githubusercontent.com/alec-chicherini/development-scripts/refs/heads/main/cmake/install_cmake.sh 2>/dev/null | bash

FROM ubuntu2404_common_deps AS ubuntu2404_userver_2_7
ENV DEBIAN_FRONTEND=noninteractive

RUN DEPS_FILE="https://raw.githubusercontent.com/userver-framework/userver/refs/heads/develop/scripts/docs/en/deps/ubuntu-24.04.md" && \
    apt install --allow-downgrades -y $(wget -q -O - ${DEPS_FILE})

RUN wget https://github.com/userver-framework/userver/releases/download/v2.7/ubuntu24.04-libuserver-all-dev_2.7_amd64.deb && \
    dpkg -i ubuntu24.04-libuserver-all-dev_2.7_amd64.deb

FROM ubuntu2404_userver_2_7 AS wordle_server_game_build
RUN apt install -y protobuf-compiler libprotoc-dev
COPY . /wordle-server-game
RUN cd /wordle-server-game && mkdir build && cd build && \
    cmake .. -DCMAKE_C_COMPILER=gcc-13 -DCMAKE_CXX_COMPILER=g++-13 && \
    cmake --build . && \
    cpack . && \
    mkdir /result && \
    cp *.deb /result/

FROM wordle_server_game_build AS wordle_server_game_run
RUN dpkg -i /result/*.deb

ENTRYPOINT ["wordle-server-game", "--config", "/etc/wordle-server-game/static_config.yaml" ]