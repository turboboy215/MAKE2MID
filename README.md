# MAKE2MID
Make Software (NES/GB/GG) to MIDI converter

This tool converts music (and sound effects) from NES, Game Boy, and Game Gear games using Make Software's sound engine to MIDI format. This is my first new MIDI converter in a while, since I have moved most development to GB2MID. This code is also implemented into the tool, but is now available as a standalone application in order to also support other systems.

It works with ROM images. To use it, you must specify the name of the ROM followed by the number of the bank containing the sound data (in hex). The system will automatically be detected.
For games that contain multiple banks of music, you must run the program multiple times specifying where each different bank is located. However, in order to prevent files from being overwritten, the MIDI files from the previous bank must either be moved to a separate folder or renamed.
An optional flag "P" can also be used to adjust the tempo for PAL NES.

Examples:
* MAKE2MID "Darkwing Duck (E).gb" 4
* MAKE2MID "Duck Tales 2 (E) [!].nes" 1 P
* MAKE2MID "Super Momotarou Dentetsu II (J) [!].gb" 7
* MAKE2MID "Super Momotarou Dentetsu II (J) [!].gb" 8

Supported games:

NES:

* Chip 'n Dale: Rescue Rangers 2
* DuckTales 2
* F1 Circus
* Fuzzical Fighter
* Gold Medal Challenge '92
* Mahjong Taisen

Game Boy:
* Adventure Island
* Booby Boys
* Darkwing Duck
* DuckTales 2
* Honmei Boy
* Milon's Secret Castle
* Momotarou Densetsu Jr.
* Nichibutsu Mahjong: Yoshimoto Gejijou
* Super Momotarou Densetsu II

Game Gear:
* Super Momotarou Densetsu III

## To do:
  * Panning support
  * NSF/GBS file support




  
