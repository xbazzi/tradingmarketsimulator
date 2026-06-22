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
	cmake --build $(BUILD_DIR) -j 8

# Build only the "options" target
options: configure ## Build options target
	cmake --build $(BUILD_DIR) --target options -j 8
build-test: configure ## Build test target
	cmake --build $(BUILD_DIR) --target unit_tests -j 8
md_consumer: configure ## Build md_consumer binary
	cmake --build $(BUILD_DIR) --target md_consumer -j 8
md_generator: configure ## Build md_generator binary
	cmake --build $(BUILD_DIR) --target md_generator -j 8

# Configure step (only runs once unless CMakeLists.txt changes)
$(BUILD_DIR)/CMakeCache.txt: CMakeLists.txt ## Configure build directory
	cmake -S . -B $(BUILD_DIR) 

configure: $(BUILD_DIR)/CMakeCache.txt 

# Clean build files
clean:
	rm -rf $(BUILD_DIR)
