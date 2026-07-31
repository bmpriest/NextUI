IMAGE_NAME ?= nextui-my282-toolchain:local
CONTAINER_RUNTIME ?= $(shell command -v docker 2>/dev/null || command -v podman 2>/dev/null)

.PHONY: clean

.build: Dockerfile toolchain-armv7.cmake
	@test -n "$(CONTAINER_RUNTIME)" || (echo "docker or podman is required" >&2; exit 1)
	$(CONTAINER_RUNTIME) build -t $(IMAGE_NAME) .
	touch .build

clean:
	@test -n "$(CONTAINER_RUNTIME)" || exit 0
	$(CONTAINER_RUNTIME) image rm $(IMAGE_NAME) 2>/dev/null || true
	rm -f .build
