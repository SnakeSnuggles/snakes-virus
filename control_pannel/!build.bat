mkdir build
cd build

:: Generate Makefiles using NMake and specify Release build
cmake .. -G "NMake Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release

:: Build the project (no --config needed with NMake)
cmake --build .

cd ..