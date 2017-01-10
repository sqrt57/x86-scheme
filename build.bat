mkdir build
del /Q build\*.*
fasm-win\fasm src\x86-scheme-win32.asm build\x86-scheme.exe -s build\x86-scheme-win32.fas
fasm-win\fasm src\x86-scheme-linux32.asm build\x86-scheme -s build\x86-scheme-linux32.fas

