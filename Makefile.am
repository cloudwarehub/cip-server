AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = cip-server
cip_server_SOURCES=./src/cip_server.c \
    ./src/cip_window.c \
    ./src/cip_common.c \
    ./src/cip_session.c \
    ./src/cip_channel.c \
    ./src/ringbuf.c
LIBS += -lpthread -luv -lx264 -lxcb -lxcb-composite -lxcb-damage -lxcb-xtest -lxcb-icccm
AM_CPPFLAGS = -I./include