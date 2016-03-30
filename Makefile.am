AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = cip-server
cip_server_SOURCES=./src/cip_server.c \
    ./src/cip_window.c \
    ./src/cip_common.c \
    ./src/cip_session.c
LIBS += -lpthread -luv -lx264 -lxcb -lxcb-composite -lxcb-damage
AM_CPPFLAGS = -I./include