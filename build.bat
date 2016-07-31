mkdir build
del /Q build\*.*
fasm-win\fasm src\x86-scheme-win32.asm build\x86-scheme.exe^
    -s build\x86-scheme-win32.fas
rem utils\listing.exe build\x86-scheme-win32.fas build\x86-scheme-win32.lst
rem utils\prepsrc.exe build\x86-scheme-win32.fas build\x86-scheme-win32.pre
rem etils\symbols.exe build\x86-scheme-win32.fas build\x86-scheme-win32.sym
rem utils\pedump /a build\x86-scheme.exe >build\x86-scheme-win32.txt

fasm-win\fasm src\x86-scheme-linux32.asm build\x86-scheme^
    -s build\x86-scheme-linux32.fas
rem utils\listing.exe build\x86-scheme-linux32.fas build\x86-scheme-linux32.lst
rem utils\prepsrc.exe build\x86-scheme-linux32.fas build\x86-scheme-linux32.pre
rem utils\symbols.exe build\x86-scheme-linux32.fas build\x86-scheme-linux32.sym

