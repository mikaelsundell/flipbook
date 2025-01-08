echo "Building flipman ..."
export PATH=$PATH:/Users/mikaelsundell/Qt6.8/Tools/CMake/CMake.app/Contents/bin
cmake .. -DCMAKE_PREFIX_PATH=/Users/mikaelsundell/Qt6.8/6.8.1/macos -G Xcode
