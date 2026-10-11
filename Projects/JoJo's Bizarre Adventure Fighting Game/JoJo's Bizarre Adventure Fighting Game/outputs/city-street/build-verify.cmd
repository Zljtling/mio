@echo off
call "D:\Tools\Programs\Microsoft Visual Studio\18\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
cl /nologo /std:c++20 /EHsc /MT /O2 /utf-8 /DUNICODE /D_UNICODE /Fe:"outputs\city-street\verify-city.exe" /Fo:"outputs\city-street\verify-city.obj" "outputs\city-street\verify-city.cpp" /link /LIBPATH:"D:\Tools\Programs\Microsoft Visual Studio\18\VC\Auxiliary\VS\lib\x64" user32.lib gdi32.lib comdlg32.lib shell32.lib ole32.lib
