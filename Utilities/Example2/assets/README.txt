Place your asset files in this folder. (textures and shaders)
On Android, the contents of this folder will be included in your APK file,
and can be read using "fopen". Internally, fopen has been re-directed to
read from this folder, using the AssetManager, but will be in read-only mode.


