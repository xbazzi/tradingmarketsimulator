BUILD_DIR := build
help: ## Show this help message
	@echo "Available build targets"
	@echo ""
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2}'
	@echo ""
	@echo "For example, to build just the options executable:"
	@echo "      > make options"              

# Default target: build everything
all: configure ## Build all targets
	cmake --build $(BUILD_DIR)

options: configure ## Build options target
	cmake --build $(BUILD_DIR) --target options
tests: configure ## Build and run unit tests (use FILTER=pattern to match specific tests)
	cmake --build $(BUILD_DIR) --target unit_tests && $(BUILD_DIR)/bin/unit_tests $(if $(FILTER),--gtest_filter=$(FILTER),)
md_consumer: configure ## Build md_consumer binary
	cmake --build $(BUILD_DIR) --target md_consumer
md_generator: configure ## Build md_generator binary
	cmake --build $(BUILD_DIR) --target md_generator
mms: configure md_generator md_consumer

# Configure step (only runs once unless CMakeLists.txt changes)
$(BUILD_DIR)/CMakeCache.txt: CMakeLists.txt ## Configure build directory
	@NINJA=$$(which ninja 2>/dev/null); \
	if [ -z "$$NINJA" ]; then \
		echo "Error: 'ninja' not found. Run: nix develop (or ensure direnv is active)."; \
		exit 1; \
	fi; \
	if [ -f CMakeUserPresets.json ]; then \
		cmake --preset local; \
	else \
		cmake -S . -B $(BUILD_DIR) -G Ninja -DCMAKE_MAKE_PROGRAM=$$NINJA -DCMAKE_COLOR_DIAGNOSTICS=ON; \
	fi

configure: $(BUILD_DIR)/CMakeCache.txt 

# Clean build files
clean:
	rm -rf $(BUILD_DIR)
