.DEFAULT_GOAL := build
BUILD_DIR := build
CMAKE := cmake

build:
	$(CMAKE) -S . -B $(BUILD_DIR) -G Ninja -DCMAKE_BUILD_TYPE=Release
	$(CMAKE) --build $(BUILD_DIR)

debug:
	$(CMAKE) -S . -B $(BUILD_DIR) -G Ninja -DCMAKE_BUILD_TYPE=Debug
	$(CMAKE) --build $(BUILD_DIR)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

sanitize:
	$(CMAKE) -S . -B $(BUILD_DIR)-sanitize -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
	$(CMAKE) --build $(BUILD_DIR)-sanitize

clean:
	cmake -E remove_directory $(BUILD_DIR)

.PHONY: build debug test sanitize clean
