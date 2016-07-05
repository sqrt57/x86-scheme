mkdir build
del /Q build\*.*
fasm-win\fasm src\enscheme-win32.asm build\enscheme.exe^
    -s build\enscheme-win32.fas
utils\listing.exe build\enscheme-win32.fas build\enscheme-win32.lst
utils\prepsrc.exe build\enscheme-win32.fas build\enscheme-win32.pre
utils\symbols.exe build\enscheme-win32.fas build\enscheme-win32.sym
pedump\pedump /a build\enscheme.exe >build\enscheme-win32.txt

fasm-win\fasm src\enscheme-linux32.asm build\enscheme^
    -s build\enscheme-linux32.fas
utils\listing.exe build\enscheme-linux32.fas build\enscheme-linux32.lst
utils\prepsrc.exe build\enscheme-linux32.fas build\enscheme-linux32.pre
utils\symbols.exe build\enscheme-linux32.fas build\enscheme-linux32.sym

