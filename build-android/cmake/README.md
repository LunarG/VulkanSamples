Build Validation Layers with Android CMake
=========================================
Gradle's CMake library project in this directory builds layers into AAR; the project could be directly
added into apps gradle projects:
- settings.gradle:  add <include "path/to/the/this/lib/:layerLib"
- app's build.gradle:
```java
dependencies {
    // force debug layer lib for packing
    compile project(path: ':layerlib', configuration: 'debug')
}
```

Build Instructions
-----------------
- ../build-android/update_external_source_android.sh ( or update_external_source_android.bat ) must run first to generate shaderc libs
- ../android-generate.sh (or android-generate.bat) to generate common headers and source codes
- set up ANDROID_HOME/ANDROID_NDK_HOME for your SDK/NDK location
- <./gradlew assemble>, and find aar inside <layerlib/build/outputs/aar>


