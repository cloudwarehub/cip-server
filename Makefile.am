AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = cip-server
cip_server_SOURCES=./src/cip_server.c
#SUBDIRS = vendor/libyuv src
LIBS += -luv -lx264 -lxcb -lxcb-composite -lxcb-damage
AM_CPPFLAGS = -I./include