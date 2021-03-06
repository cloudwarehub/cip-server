FROM ubuntu:14.04
MAINTAINER guodong <gd@tongjo.com>
RUN apt-get update
RUN apt-get install -y xorg xserver-xorg-video-dummy wget git make autoconf automake libtool websockify python-pkg-resources
RUN apt-get install -y x264 libxcb1 libxcb-composite0 libxcb-xtest0 libxcb-keysyms1 libxcb-icccm4 libxcb-damage0 libxcb-util0
RUN git clone https://github.com/libuv/libuv.git && cd libuv && sh ./autogen.sh && ./configure && make && make install && ln -s /usr/local/lib/libuv.so.1 /usr/lib/
RUN wget https://gist.githubusercontent.com/guodong/91b631bdfa42e5e72f21/raw/8c942883b96e2996fa9cf541c6bf6150a1c3afb9/xorg-dummy.conf -O /root/xorg.conf
ADD ./start.sh /root/start.sh
ADD ./dest/cip-server /root/cip-server
RUN chmod u+x /root/start.sh
EXPOSE 5998 5999 6000
ENV DISPLAY :0
CMD /root/start.sh