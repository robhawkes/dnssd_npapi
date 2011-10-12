CONFIGURATION=MinSizeRel # Debug Release MinSizeRel RelWithDebInfo
UNAME:=$(shell (uname -o 2>/dev/null||uname -s 2>/dev/null)|tr 'A-Z/' 'a-z-')
include $(UNAME).mk

all: compile

FireBreath:
	git submodule update --recursive --init

build: FireBreath
	cp FireBreath/gen_templates/config.h DNSSDPlugin/config.h
	echo '#define MOZILLA_CHROME_URI "$${MOZILLA_CHROME_URI}"' >> DNSSDPlugin/config.h
	./FireBreath/${BUILD_CMD}

compile: build
	cd build && ${COMPILE_CMD}

clean:
	rm -fr build DNSSDPlugin/config.h
