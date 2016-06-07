#!/usr/bin/env python3
#
# Copyright (c) 2016 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import ctypes
import json
import os
import platform
import sys
import xml.etree.ElementTree

if platform.system() == "Windows":
    VKAPI_DLL = ctypes.windll
    VKAPI_FUNCTYPE = ctypes.WINFUNCTYPE
else:
    VKAPI_DLL = ctypes.cdll
    VKAPI_FUNCTYPE = ctypes.CFUNCTYPE

# Vulkan types

VkInstance = ctypes.c_void_p
VkPhysicalDevice = ctypes.c_void_p
VkDevice = ctypes.c_void_p
VkResult = ctypes.c_int


class VkLayerProperties(ctypes.Structure):
    _fields_ = [("c_layerName", ctypes.c_char * 256),
                ("c_specVersion", ctypes.c_uint32),
                ("c_implementationVersion", ctypes.c_uint32),
                ("c_description", ctypes.c_char * 256)]

    def layer_name(self):
        return self.c_layerName.decode()

    def spec_version(self):
        return "%d.%d.%d" % (
            self.c_specVersion >> 22,
            (self.c_specVersion >> 12) & 0x3ff,
            self.c_specVersion & 0xfff)

    def implementation_version(self):
        return str(self.c_implementationVersion)

    def description(self):
        return self.c_description.decode()

    def __eq__(self, other):
        return (self.c_layerName == other.c_layerName and
                self.c_specVersion == other.c_specVersion and
                self.c_implementationVersion == other.c_implementationVersion and
                self.c_description == other.c_description)


class VkExtensionProperties(ctypes.Structure):
    _fields_ = [("c_extensionName", ctypes.c_char * 256),
                ("c_specVersion", ctypes.c_uint32)]

    def extension_name(self):
        return self.c_extensionName.decode()

    def spec_version(self):
        return str(self.c_specVersion)

# Vulkan commands

PFN_vkVoidFunction = VKAPI_FUNCTYPE(None)
PFN_vkEnumerateInstanceExtensionProperties = VKAPI_FUNCTYPE(
    VkResult, ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(VkExtensionProperties))
PFN_vkEnumerateDeviceExtensionProperties = VKAPI_FUNCTYPE(
    VkResult, VkPhysicalDevice, ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(VkExtensionProperties))
PFN_vkEnumerateInstanceLayerProperties = VKAPI_FUNCTYPE(
    VkResult, ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(VkLayerProperties))
PFN_vkEnumerateDeviceLayerProperties = VKAPI_FUNCTYPE(
    VkResult, VkPhysicalDevice, ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(VkLayerProperties))
PFN_vkGetInstanceProcAddr = VKAPI_FUNCTYPE(
    PFN_vkVoidFunction, VkInstance, ctypes.c_char_p)
PFN_vkGetDeviceProcAddr = VKAPI_FUNCTYPE(
    PFN_vkVoidFunction, VkDevice, ctypes.c_char_p)


class Layer(object):

    def __init__(self, *args):
        self.props = args[0]
        self.is_global = args[1]
        self.instance_extensions = args[2]
        self.device_extensions = args[3]
        self.gipa_name = args[4]
        self.gdpa_name = args[5]


class LayerLibrary(object):

    def __init__(self, path):
        self.library = None
        self.version = 0

        self._load(path)
        self._negotiate_version()

    def introspect(self):
        if self.version == 0:
            layers = self._enumerate_layers_v0()
        else:
            raise RuntimeError("unsupported v%d library" % self.version)

        return layers

    def _load(self, path):
        try:
            abspath = os.path.abspath(path)
            self.library = VKAPI_DLL.LoadLibrary(abspath)
        except OSError:
            raise RuntimeError("failed to load library")

    def _unload(self):
        # no clean way to unload
        pass

    def _negotiate_version(self):
        # only v0
        self.version = 0

    def _enumerate_properties_errcheck_v0(self, result, func, args):
        if isinstance(func, PFN_vkEnumerateInstanceLayerProperties):
            func_name = "vkEnumerateInstanceLayerProperties"
        elif isinstance(func, PFN_vkEnumerateDeviceLayerProperties):
            func_name = "vkEnumerateDeviceLayerProperties"
        elif isinstance(func, PFN_vkEnumerateInstanceExtensionProperties):
            func_name = "vkEnumerateInstanceExtensionProperties"
        elif isinstance(func, PFN_vkEnumerateDeviceExtensionProperties):
            func_name = "vkEnumerateDeviceExtensionProperties"
        else:
            raise AssertionError("unexpected vkEnumerate*Properties call")

        if result != 0:
            raise RuntimeError(func_name + " failed with " + str(result))

        # pProperties and pCount mismatch
        if args[-1] and len(args[-1]) != args[-2].value:
            raise RuntimeError("invalid pCount returned in " + func_name)

        return args[-1]

    def _enumerate_properties_prototype_v0(self, func_name):
        prototypes = {
            "vkEnumerateInstanceLayerProperties":
            PFN_vkEnumerateInstanceLayerProperties,
            "vkEnumerateDeviceLayerProperties":
            PFN_vkEnumerateDeviceLayerProperties,
            "vkEnumerateInstanceExtensionProperties":
            PFN_vkEnumerateInstanceExtensionProperties,
            "vkEnumerateDeviceExtensionProperties":
            PFN_vkEnumerateDeviceExtensionProperties,
        }
        prototype = prototypes[func_name]

        try:
            proc = prototype((func_name, self.library))
        except AttributeError:
            raise RuntimeError(func_name + " is missing")

        proc.errcheck = self._enumerate_properties_errcheck_v0

        return proc

    def _get_gipa_name_v0(self, layer_name, can_fallback):
        names = [layer_name + "GetInstanceProcAddr"]
        if can_fallback:
            names.append("vkGetInstanceProcAddr")

        for name in names:
            try:
                PFN_vkGetInstanceProcAddr((name, self.library))
                return name
            except AttributeError:
                pass

        raise RuntimeError(" or ".join(names) + " is missing")

    def _get_gdpa_name_v0(self, layer_name, can_fallback):
        names = [layer_name + "GetDeviceProcAddr"]
        if can_fallback:
            names.append("vkGetDeviceProcAddr")

        for name in names:
            try:
                PFN_vkGetDeviceProcAddr((name, self.library))
                return name
            except AttributeError:
                pass

        raise RuntimeError(" or ".join(names) + " is missing")

    def _enumerate_layers_v0(self):
        tmp_count = ctypes.c_uint32()

        # enumerate instance layers
        enumerate_instance_layer_properties = self._enumerate_properties_prototype_v0(
            "vkEnumerateInstanceLayerProperties")
        enumerate_instance_layer_properties(tmp_count, None)
        p_props = enumerate_instance_layer_properties(
            tmp_count, (VkLayerProperties * tmp_count.value)())

        # enumerate device layers
        enumerate_device_layer_properties = self._enumerate_properties_prototype_v0(
            "vkEnumerateDeviceLayerProperties")
        enumerate_device_layer_properties(None, tmp_count, None)
        dev_p_props = enumerate_device_layer_properties(
            None, tmp_count, (VkLayerProperties * tmp_count.value)())

        # there must not be device-only layers
        for props in dev_p_props:
            if props not in p_props:
                raise RuntimeError(
                    "unexpected device-only layer " + props.layer_name())

        layers = []
        for props in p_props:
            is_global = (props in dev_p_props)

            # enumerate instance extensions
            enumerate_instance_extension_properties = self._enumerate_properties_prototype_v0(
                "vkEnumerateInstanceExtensionProperties")
            enumerate_instance_extension_properties(
                props.c_layerName, tmp_count, None)
            instance_extensions = enumerate_instance_extension_properties(
                props.c_layerName,
                tmp_count,
                (VkExtensionProperties * tmp_count.value)())

            gipa_name = self._get_gipa_name_v0(
                props.layer_name(),
                len(p_props) == 1)

            if is_global:
                # enumerate device extensions
                enumerate_device_extension_properties = self._enumerate_properties_prototype_v0(
                    "vkEnumerateDeviceExtensionProperties")
                enumerate_device_extension_properties(
                    None, props.c_layerName, tmp_count, None)
                device_extensions = enumerate_device_extension_properties(
                    None,
                    props.c_layerName,
                    tmp_count,
                    (VkExtensionProperties * tmp_count.value)())

                gdpa_name = self._get_gdpa_name_v0(
                    props.layer_name(),
                    len(p_props) == 1)
            else:
                device_extensions = None
                gdpa_name = None

            layers.append(
                Layer(props, is_global, instance_extensions, device_extensions, gipa_name, gdpa_name))

        return layers


def serialize_layers(layers, path, ext_cmds):
    data = {}
    data["file_format_version"] = '1.0.0'

    for idx, layer in enumerate(layers):
        layer_data = {}

        layer_data["name"] = layer.props.layer_name()
        layer_data["api_version"] = layer.props.spec_version()
        layer_data[
            "implementation_version"] = layer.props.implementation_version()
        layer_data["description"] = layer.props.description()

        layer_data["type"] = "GLOBAL" if layer.is_global else "INSTANCE"

        # TODO more flexible
        layer_data["library_path"] = os.path.join(".", os.path.basename(path))

        funcs = {}
        if layer.gipa_name != "vkGetInstanceProcAddr":
            funcs["vkGetInstanceProcAddr"] = layer.gipa_name
        if layer.is_global and layer.gdpa_name != "vkGetDeviceProcAddr":
            funcs["vkGetDeviceProcAddr"] = layer.gdpa_name
        if funcs:
            layer_data["functions"] = funcs

        if layer.instance_extensions:
            exts = [{
                "name": ext.extension_name(),
                "spec_version": ext.spec_version(),
            } for ext in layer.instance_extensions]
            layer_data["instance_extensions"] = exts

        if layer.device_extensions:
            exts = []
            for ext in layer.device_extensions:
                try:
                    cmds = ext_cmds[ext.extension_name()]
                except KeyError:
                    raise RuntimeError(
                        "unknown device extension " + ext.extension_name())
                else:
                    ext_data = {}
                    ext_data["name"] = ext.extension_name()
                    ext_data["spec_version"] = ext.spec_version()
                    if cmds:
                        ext_data["entrypoints"] = cmds

                    exts.append(ext_data)

            layer_data["device_extensions"] = exts

        if idx > 0:
            data["layer.%d" % idx] = layer_data
        else:
            data["layer"] = layer_data

    return data


def dump_json(data):
    dump = json.dumps(data, indent=4, sort_keys=True)

    # replace "layer.<idx>" by "layer"
    lines = dump.split("\n")
    for line in lines:
        if line.startswith("    \"layer.") and line.endswith("\": {"):
            line = "    \"layer\": {"
        print(line)


def parse_vk_xml(path):
    """Parse vk.xml to get commands added by extensions."""
    tree = xml.etree.ElementTree.parse(path)
    extensions = tree.find("extensions")

    ext_cmds = {}
    for ext in extensions.iter("extension"):
        if ext.attrib["supported"] != "vulkan":
            continue

        cmds = []
        for cmd in ext.iter("command"):
            cmds.append(cmd.attrib["name"])

        ext_cmds[ext.attrib["name"]] = cmds

    return ext_cmds


def add_custom_ext_cmds(ext_cmds):
    """Add commands added by in-development extensions."""
    # VK_LAYER_LUNARG_basic
    ext_cmds["vkLayerBasicEXT"] = ["vkLayerBasicEXT"]


def main():
    default_vk_xml = sys.path[0] + "/vk.xml" if sys.path[0] else "vk.xml"

    parser = argparse.ArgumentParser(description="Introspect a layer library.")
    parser.add_argument(
        "-x", dest="vk_xml", default=default_vk_xml, help="Path to vk.xml")
    parser.add_argument(
        "layer_libs", metavar="layer-lib", nargs="+", help="Path to a layer library")
    args = parser.parse_args()

    try:
        ext_cmds = parse_vk_xml(args.vk_xml)
    except Exception as e:
        print("failed to parse %s: %s" % (args.vk_xml, e))
        sys.exit(-1)

    add_custom_ext_cmds(ext_cmds)

    for path in args.layer_libs:
        try:
            ll = LayerLibrary(path)
            layers = ll.introspect()
            data = serialize_layers(layers, path, ext_cmds)
            dump_json(data)
        except RuntimeError as err:
            print("skipping %s: %s" % (path, err))

if __name__ == "__main__":
    main()
