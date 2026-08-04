REQUIRES_FILEMGR = other/NextCommander
NEXTCOMMANDER_COMMIT = aa542cb714891b81dedb1e683067535d187d1eee

.PHONY: early clean

early: $(REQUIRES_FILEMGR)/.my282-patched
	$(MAKE) -C libmstick
	$(MAKE) -C $(REQUIRES_FILEMGR) PLATFORM=my282

$(REQUIRES_FILEMGR)/.my282-patched:
	mkdir -p other
	git clone https://github.com/LoveRetro/NextCommander.git $(REQUIRES_FILEMGR)
	cd $(REQUIRES_FILEMGR) && git checkout $(NEXTCOMMANDER_COMMIT)
	cd $(REQUIRES_FILEMGR) && git apply ../../nextcommander.patch
	touch $@

clean:
	$(MAKE) -C libmstick clean
	if [ -d "$(REQUIRES_FILEMGR)" ]; then \
		$(MAKE) -C $(REQUIRES_FILEMGR) clean; \
	fi
