# workspace (all)

###########################################################

ifeq (,$(PLATFORM))
PLATFORM=$(UNION_PLATFORM)
endif

ifeq (,$(PLATFORM))
$(error please specify PLATFORM, eg. PLATFORM=trimui make)
endif

###########################################################

.PHONY: all

all:
ifeq ($(PLATFORM), desktop)
	cd ./$(PLATFORM) && make early # eg. other libs
	cd ./all/nextui/ && make
	cd ./all/minarch/ && make
	#cd ./all/libbatmondb/ && make
	#cd ./all/battery/ && make
	cd ./all/clock/ && make
	#cd ./all/batmon/ && make
	#cd ./all/libgametimedb/ && make
	#cd ./all/gametimectl/ && make
	#cd ./all/gametime/ && make
	cd ./all/minput/ && make
	cd ./all/nextval/ && make
	cd ./all/settings/ && make
else ifeq ($(PLATFORM), my282)
	cd ./$(PLATFORM)/libmsettings && make
	cd ./$(PLATFORM) && make early
	cd ./$(PLATFORM)/keymon && make
	cd ./all/nextui/ && make
	cd ./all/minarch/ && make
	cd ./all/libgametimedb/ && make
	cd ./all/gametimectl/ && make
	cd ./all/gametime/ && make
	cd ./all/libbatmondb/ && make
	cd ./all/batmon/ && make
	cd ./all/battery/ && make
	cd ./all/clock/ && make
	cd ./all/minput/ && make
	cd ./all/minuilist/ && make
	cd ./all/settings/ && make
	cd ./$(PLATFORM)/package && make
else 
ifeq ($(PLATFORM), tg5040)
	cd ./$(PLATFORM)/rfkill && make all
	cd ./$(PLATFORM)/btmanager && make all
	cd ./$(PLATFORM)/poweroff_next/ && make
endif
	cd ./$(PLATFORM)/libmsettings && make
	cd ./$(PLATFORM) && make early # eg. other libs
	cd ./$(PLATFORM)/keymon && make
	cd ./all/nextui/ && make
	cd ./all/minarch/ && make
	cd ./all/battery/ && make
	cd ./all/clock/ && make
	cd ./all/libbatmondb/ && make
	cd ./all/batmon/ && make
	cd ./all/libgametimedb/ && make
	cd ./all/gametimectl/ && make
	cd ./all/gametime/ && make
	cd ./all/minput/ && make
	cd ./all/syncsettings/ && make
	cd ./all/nextval/ && make
	cd ./all/settings/ && make
	cd ./all/ledcontrol/ && make
	cd ./all/bootlogo/ && make
	cd ./all/audiomon && make all
	cd ./all/show2/ && make
endif
ifneq ($(PLATFORM),my282)
ifdef COMPILE_CORES
	make cores
else
	echo "skipping core build"
endif
	cd ./$(PLATFORM) && make
endif

cores:
	cd ./$(PLATFORM)/cores && make

core:
ifndef CORE
	$(error CORE is not set)
endif
	cd ./$(PLATFORM)/cores && make $(CORE)

clean:
ifeq ($(PLATFORM),my282)
	cd ./$(PLATFORM) && make clean
	cd ./$(PLATFORM)/libmsettings && make clean
	cd ./$(PLATFORM)/keymon && make clean
	cd ./all/nextui/ && make clean
	cd ./all/minarch/ && make clean
	cd ./all/libgametimedb/ && make clean
	cd ./all/gametimectl/ && make clean
	cd ./all/gametime/ && make clean
	cd ./all/batmon/ && make clean
	cd ./all/battery/ && make clean
	cd ./all/libbatmondb/ && make clean
	cd ./all/clock/ && make clean
	cd ./all/minput/ && make clean
	cd ./all/minuilist/ && make clean
	cd ./all/settings/ && make clean
	cd ./$(PLATFORM)/cores && make clean
	cd ./$(PLATFORM)/package && make clean
else
ifneq ($(PLATFORM), desktop)
	cd ./$(PLATFORM)/keymon && make clean
	cd ./all/syncsettings/ && make clean
	cd ./all/ledcontrol/ && make clean
	cd ./all/bootlogo/ && make clean
	cd ./all/show2/ && make clean
	cd ./all/batmon/ && make clean
ifeq ($(PLATFORM), tg5040)
	cd ./$(PLATFORM)/rfkill && make clean
	cd ./$(PLATFORM)/btmanager && make clean
	cd ./$(PLATFORM)/poweroff_next/ && make clean
endif
endif
	cd ./$(PLATFORM)/libmsettings && make clean
	cd ./all/nextui/ && make clean
	cd ./all/minarch/ && make clean
	cd ./all/battery/ && make clean
	cd ./all/clock/ && make clean
	cd ./all/libbatmondb/ && make clean
	cd ./all/libgametimedb/ && make clean
	cd ./all/gametimectl/ && make clean
	cd ./all/gametime/ && make clean
	cd ./all/minput/ && make clean
	cd ./all/nextval/ && make clean
	cd ./all/settings/ && make clean
	cd ./all/audiomon/ && make clean
	cd ./$(PLATFORM) && make clean
endif
