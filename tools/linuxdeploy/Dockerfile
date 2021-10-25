FROM ubuntu:bionic

RUN apt-get update
RUN apt-get install -y git build-essential mingw-w64 clang curl python python3 pkg-config \
    libcurl4-openssl-dev libfreetype6-dev libx11-dev libxext-dev \
    libxrandr-dev libxcomposite-dev libxinerama-dev libxcursor-dev \
    libjack-dev libasound2-dev lv2-dev liblilv-dev libsuil-dev ladspa-sdk \
    libcurl4-openssl-dev libgtk2.0-dev \
    libfuse2 imagemagick \
    lua-ldoc

RUN cd / && git clone --depth 1 https://github.com/kushview/depends
RUN cd /depends && git reset --hard origin/master && rm -rf .git
RUN cd /depends && make HOST=x86_64-pc-linux-gnu
RUN cd /depends && make HOST=x86_64-w64-mingw32

RUN update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix; \
    update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix

RUN curl -L https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage > /usr/bin/linuxdeploy-x86_64.AppImage && \
    chmod +x /usr/bin/linuxdeploy-x86_64.AppImage && \
    ln -s /usr/bin/linuxdeploy-x86_64.AppImage /usr/bin/linuxdeploy

# ADD . /element/
