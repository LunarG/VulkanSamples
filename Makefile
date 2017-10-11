# top-level Linux makefile for
# https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers
# mikew@lunarg.com

LVL_TARGET_EXE = vk_layer_validation_tests
#LVL_TARGET_EXE = vk_loader_validation_tests

#LVL_GTEST_ARGS = --gtest_filter=VkPositiveLayerTest.PSOPolygonModeValid
#LVL_GTEST_ARGS = --gtest_list_tests

LVL_EXTERNAL_DIR = external
LVL_BUILD_DIR    = BUILD
LVL_TESTS_DIR    = $(LVL_BUILD_DIR)/tests
LVL_MAKEFILE     = $(LVL_BUILD_DIR)/Makefile
LVL_TARGET       = $(LVL_TESTS_DIR)/$(LVL_TARGET_EXE)
LVL_LAYER_PATH   = $(PWD)/$(LVL_BUILD_DIR)/layers


all: $(LVL_TARGET)

$(LVL_EXTERNAL_DIR):
	./update_external_sources.sh

$(LVL_BUILD_DIR): $(LVL_EXTERNAL_DIR)
	mkdir $@

$(LVL_MAKEFILE) $(LVL_TESTS_DIR): $(LVL_BUILD_DIR)
	cd $(LVL_BUILD_DIR) && cmake ..

$(LVL_TARGET): $(LVL_MAKEFILE)
	cd $(LVL_BUILD_DIR) && $(MAKE)


t test tests:
	cd $(LVL_TESTS_DIR) && VK_LAYER_PATH=$(LVL_LAYER_PATH) ./$(LVL_TARGET_EXE) $(LVL_GTEST_ARGS)


clean:
	-rm -rf $(LVL_TARGET)

clobber: clean
	-rm -rf $(LVL_BUILD_DIR)

nuke: clobber
	-rm -rf $(LVL_EXTERNAL_DIR)

# vim: set ts=8 noet ic ai:
