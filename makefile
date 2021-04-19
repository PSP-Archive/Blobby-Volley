
TARGET = Blobby
OBJS = main.o

INCDIR = 
CFLAGS = -G0 -Wall -O2 
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =
STDLIBS= -losl -lpng -lz \
		-lpspsdk -lpspctrl -lpspumd -lpsprtc -lpspaudiolib -lpspaudio \
		-lpspmpeg -lpspaudiocodec -lstdc++ -lpspnet_adhocmatching -lpspnet_adhoc -lpspnet_adhocctl \
		-lpspwlan -lpspnet -lpspusb -lpspusbstor -lpsppower -lpspgum -lpspgu -lm
LIBS=$(STDLIBS)$(YOURLIBS)


EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Blobby Volley For PSP
PSP_EBOOT_ICON = ICON.png


PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
