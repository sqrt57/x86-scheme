mkdir build
del build\enscheme.exe
del build\enscheme
fasm-win\fasm src\enscheme-win32.asm build\enscheme.exe
fasm-win\fasm src\enscheme-linux32.asm build\enscheme

