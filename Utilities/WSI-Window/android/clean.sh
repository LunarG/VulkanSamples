#!/usr/bin/env bash

# # Run this script before importing this project into Android Studio.
# It delete's Android Studio config files which contain fixed paths,
# to avoid Gradle errors when re-importing the project at a new location.
# (Dont worry, Android Studio regenerates the deleted files on import.)

rm -r gen
rm -r build
rm -r .gradle
rm -r .idea
rm Android.iml
rm local.properties

rm -r app/build
rm -r app/.externalNativeBuild
rm -r app/.idea
rm app/app.iml
rm app/local.properties
