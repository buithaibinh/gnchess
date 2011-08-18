# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
#   << cnchess >>
#
#     Created on: 2009-9-16
#         Author: Thor Qin
#          EMail: thor.qin@gmail.com
#
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Depends: thorlib(> 0.1), gtkmm-2.4, librsvg-2.0

CC = gcc
# CFLAGS = -DNDEBUG -O2 -Wall
CFLAGS = -g -Wall \
	`pkg-config gtkmm-2.4 --cflags` \
	`pkg-config librsvg-2.0 --cflags`

OBJECTS = ui.o engine.o icon.o ui_data.o svg_res.o
BIN = $(DESTDIR)/usr/bin
SHARE = $(DESTDIR)/usr/share/cnchess
DESKTOP = $(DESTDIR)/usr/share/applications

INCFLAGS =
LDFLAGS =
LIBS = -lstdc++ \
	`pkg-config gtkmm-2.4 --libs` \
	`pkg-config gthread-2.0 --libs` \
	`pkg-config librsvg-2.0 --libs` \
	-ltlib


po_dirs := $(shell find ./po -maxdepth 1 -type d)
po_dirs := $(basename $(patsubst ./po%,%,$(po_dirs)))
po_dirs := $(basename $(patsubst /%,%,$(po_dirs)))
cleanpo_dirs := $(addprefix _clean_,$(po_dirs))
installpo_dirs := $(addprefix _inst_,$(po_dirs))

all: cnchess $(po_dirs)

cnchess: $(OBJECTS)
	$(CC) -o cnchess $(LDFLAGS) $(LIBS) $(OBJECTS)

.SUFFIXES:
.SUFFIXES:	.cc .o

sinclude ${OBJECTS:.o=.d}

icon.cc: icon.png
	gdk-pixbuf-csource --name=icon_inline --extern --raw icon.png > icon.cc && \
	patch -p0 < icon.patch

ui_data.cc: ui.glade
	mkres -n ui_data -z ui.glade > ui_data.cc

svg_res.cc: res.svg
	mkres -n svg_data res.svg > svg_res.cc

%.o: %.cc
	$(CC) -o $@ -c $(CFLAGS) $< $(INCFLAGS)
	
%.d: %.cc
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

# 创建po文件
$(po_dirs):
	-$(MAKE) -C ./po/$@

# 删除mo文件
$(cleanpo_dirs):
	-$(MAKE) -C ./po/$(patsubst _clean_%,%,$@) clean

# 安装mo文件
$(installpo_dirs):
	install -d $(SHARE)/locale/$(patsubst _inst_%,%,$@)/LC_MESSAGES
	install -m644 ./po/$(patsubst _inst_%,%,$@)/cnchess.mo \
		$(SHARE)/locale/$(patsubst _inst_%,%,$@)/LC_MESSAGES

clean: $(cleanpo_dirs)
	rm -f *.d *.o icon.cc svg_res.cc ui_data.cc cnchess
	
install: cnchess $(installpo_dirs)
	install -d $(BIN) $(SHARE) $(SHARE)/sound $(DESKTOP)
	install ./cnchess $(BIN)
	install -m644 ./book.dat $(SHARE)
	install -m644 ./icon.png $(SHARE)
	install -m644 ./sound/*.wav $(SHARE)/sound
	install -m644 ./cnchess.desktop $(DESKTOP)

uninstall: $(uninstallpo_dirs)
	rm -f $(BIN)/cnchess
	rm -f -r $(SHARE)
	rm -f $(DESKTOP)/cnchess.desktop

.PHONY: all
.PHONY: clean
.PHONY: install
.PHONY: uninstall
.PHONY: $(po_dirs) $(cleanpo_dirs) $(installpo_dirs)
