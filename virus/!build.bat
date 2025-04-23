@echo off

if not exist build (
    mkdir build
)
cd build

cmake .. -G "NMake Makefiles" -DOpenCV_DIR="C:\Users\daves\Projects\omega_virus\virus\libs\opencv\build\x64\vc16\lib" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -D_WIN32_WINNT=0x0601
cmake --build . --config Release

cd ..