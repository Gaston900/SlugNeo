# SLUGNEO
What is SlugNeo?

It's called SlugNeo, this project was part of the HBMAME emulator in the year 2018 – 2020.

Afterwards, it has been decided to make the  Metal Slug project would become independent in 2021, providing comprehensive technical support, fixing errors and failures, correcting incompatibilities with the NeoGeo system, etc.

Version 0.224 [HBMAME/EKMAME] is being used as the base system.

It allows users to enjoy a unique and distinctive gaming experience, having a great time exploring a wide catalog of different versions of all the franchises available on the NeoGeo MVS/AES system.

This project was dedicated with much effort and great care, it was developed with so much passion and with great enthusiasm, with such a deep dedication of interest in preserving the history in the roms that were published since the beginning of the emulation era since 1999 (Predecrypted, Decrypter, Encrypte, Earlier and Bootleg, Darksoft, Neo SD and Hack)

I am only supporting the operating systems, Windows 7, Windows 8, Windows 10 and Windows 11.

How to compile
--------------

In order to compile this version we will need the source code, for this we will locate it in the folder docs / Source Code [HBMame] / hbmame-tag224.7z. 001, once located we will begin to unzip the files, it will take a few minutes, once unzipped we will have a folder with the name hbmame-tag224.7z, we will rename it to “src”, Now we will get the latest source code of this Github container once downloaded we will begin to unzip and once finished unzipping we will select the files that we had left in the folder “scripts, src and makefile” we will copy them into the src folder, the system will ask us to replace it we will say yes.

The version used is msys64 7.2.0; if you do not have it, you can find it in the folder “docs / Build Tools / msys32-64-2017-12-26.7z.001”.

And we will apply this command to start the compilation, this command is for Windows 64-Bit system:
```
make PTR64=1 SUBTARGET=arcade OSD=winui NOWERROR=1 STRIP_SYMBOLS=1
```
And we will apply this command to start the compilation, this command is for Windows 32-Bit system:
```
make PTR64=0 SUBTARGET=arcade OSD=winui NOWERROR=1 STRIP_SYMBOLS=1
```

Open Source Software Projects
------------------------------
Although the source code is free to use, please note that the use of this code for any commercial exploitation or use of the project for fundraising purposes is prohibited.






