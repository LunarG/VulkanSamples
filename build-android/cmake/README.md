Build Validation Layers with Android CMake
=========================================
Gradle's CMake library project in this directory builds layers into AAR;
the project could be directly added into application's gradle projects

Pre-requirements
----------------
Build ShaderC binary
- Building from Github Repo source
1. cd android-build
2. ./update_external_sources_android.sh
3. ./android-generate.sh

Extra Steps if building from NDK's source tree
```
   cd ${your ANDROID_NDK_ROOT}/sources/third_party/shaderc
   ndk-build  APP_ABI=armeabi-v7a APP_STL=gnustl_static NDK_TOOLCHAIN_VERSION=clang NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk libshaderc_combined
```

Adding layer module into Android Studio application project
--------------------------------------------------------
1. app's settings.gradle, add 
```java
    include ':layerLib'
    project(':layerLib').projectDir = new File('/path/to/cmake/layerlib')
```
2. app's build.gradle:
```java
dependencies {
    // force debug layer lib for packing
    compile project(path: ':layerlib', configuration: 'debug')
}
```
BUILD_IN_NDK variable in layerlib/CMakeLists.txt could detect whether the source is
from NDK or github repo clone, and would configure the file paths accordingly

Tested
-----
  Build on Mac OS, Nexus 5X test phone. Equvilaent build scripts for Windows OS are at the same directory.

