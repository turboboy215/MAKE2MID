/*Make Software (NES/GB/GG) to MIDI converter*/
/*By Will Trowbridge*/
/*Portions based on code by ValleyBell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define bankSize 16384

FILE* rom, * mid;
long bank;
long offset;
long tablePtrLoc;
long tableOffset;
int i, j;
char outfile[1000000];
int songNum;
long seqPtrs[4];
long songPtr;
int chanMask;
long bankAmt;
int foundTable;
int smd2;
int extBank;
long base;
int curInst;
int curVol;
char string1[4];
char string2[9];
int format;
int usePALTempo;
char* argv3;

unsigned char* romData;
unsigned char* midData;
unsigned char* ctrlMidData;

long midLength;
const char MagicBytesGB[4] = { 0x73, 0x23, 0x72, 0x1A };
const char MagicBytesNES[9] = { 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x0A, 0xAA, 0xBD };
const char MagicBytesNES2[4] = { 0x60, 0x0A, 0xAA, 0xBD };
const char MagicBytesNES3[6] = { 0x60, 0x80, 0xA0, 0x0A, 0xAA, 0xBD };
const char MagicBytesGG[7] = { 0x32, 0xB7, 0xD1, 0xD1, 0xE1, 0xC9, 0x21 };

/*Function prototypes*/
unsigned short ReadLE16(unsigned char* Data);
void Write8B(unsigned char* buffer, unsigned int value);
void WriteBE32(unsigned char* buffer, unsigned long value);
void WriteBE24(unsigned char* buffer, unsigned long value);
void WriteBE16(unsigned char* buffer, unsigned int value);
unsigned int WriteNoteEvent(unsigned char* buffer, unsigned int pos, unsigned int note, int length, int delay, int firstNote, int curChan, int inst);
int WriteDeltaTime(unsigned char* buffer, unsigned int pos, unsigned int value);
void song2mid(int songNum, long ptr);

/*Convert little-endian pointer to big-endian*/
unsigned short ReadLE16(unsigned char* Data)
{
	return (Data[0] << 0) | (Data[1] << 8);
}

void Write8B(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = value;
}

void WriteBE32(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF000000) >> 24;
	buffer[0x01] = (value & 0x00FF0000) >> 16;
	buffer[0x02] = (value & 0x0000FF00) >> 8;
	buffer[0x03] = (value & 0x000000FF) >> 0;

	return;
}

void WriteBE24(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF0000) >> 16;
	buffer[0x01] = (value & 0x00FF00) >> 8;
	buffer[0x02] = (value & 0x0000FF) >> 0;

	return;
}

void WriteBE16(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = (value & 0xFF00) >> 8;
	buffer[0x01] = (value & 0x00FF) >> 0;

	return;
}

unsigned int WriteNoteEvent(unsigned char* buffer, unsigned int pos, unsigned int note, int length, int delay, int firstNote, int curChan, int inst)
{
	int deltaValue;
	deltaValue = WriteDeltaTime(buffer, pos, delay);
	pos += deltaValue;

	if (firstNote == 1)
	{
		if (curChan != 3)
		{
			Write8B(&buffer[pos], 0xC0 | curChan);
		}
		else
		{
			Write8B(&buffer[pos], 0xC9);
		}

		Write8B(&buffer[pos + 1], inst);
		Write8B(&buffer[pos + 2], 0);

		if (curChan != 3)
		{
			Write8B(&buffer[pos + 3], 0x90 | curChan);
		}
		else
		{
			Write8B(&buffer[pos + 3], 0x99);
		}

		pos += 4;
	}

	Write8B(&buffer[pos], note);
	pos++;
	Write8B(&buffer[pos], 100);
	pos++;

	deltaValue = WriteDeltaTime(buffer, pos, length);
	pos += deltaValue;

	Write8B(&buffer[pos], note);
	pos++;
	Write8B(&buffer[pos], 0);
	pos++;

	return pos;

}

int WriteDeltaTime(unsigned char* buffer, unsigned int pos, unsigned int value)
{
	unsigned char valSize;
	unsigned char* valData;
	unsigned int tempLen;
	unsigned int curPos;

	valSize = 0;
	tempLen = value;

	while (tempLen != 0)
	{
		tempLen >>= 7;
		valSize++;
	}

	valData = &buffer[pos];
	curPos = valSize;
	tempLen = value;

	while (tempLen != 0)
	{
		curPos--;
		valData[curPos] = 128 | (tempLen & 127);
		tempLen >>= 7;
	}

	valData[valSize - 1] &= 127;

	pos += valSize;

	if (value == 0)
	{
		valSize = 1;
	}
	return valSize;
}

int main(int args, char* argv[])
{
	foundTable = 0;
	smd2 = 0;
	format = 2;
	usePALTempo = 0;
	printf("Make Software (NES/GB/GG) to MIDI converter\n");
	if (args < 3)
	{
		printf("Usage: MAKE2MID <rom> <bank>\n");
		return -1;
	}
	else
	{
		if (args >= 4)
		{
			argv3 = argv[3];
			if (strcmp(argv3, "p") == 0 || strcmp(argv3, "P") == 0)
			{
				usePALTempo = 1;
			}
		}
		if ((rom = fopen(argv[1], "rb")) == NULL)
		{
			printf("ERROR: Unable to open file %s!\n", argv[1]);
			exit(1);
		}
		else
		{
			bank = strtol(argv[2], NULL, 16);
			if (bank != 1)
			{
				bankAmt = bankSize;
			}
			else
			{
				bankAmt = 0;
			}
			/*Check for NES ROM header*/
			fgets(string1, 4, rom);
			if (!memcmp(string1, "NES", 1))
			{
				fseek(rom, (((bank - 1) * bankSize)) + 0x10, SEEK_SET);
			}
			else
			{
				fseek(rom, 0x7FF0, SEEK_SET);
				fgets(string2, 9, rom);
				if (!memcmp(string2, "TMR SEGA", 1))
				{
					format = 3;
				}
				fseek(rom, ((bank - 1) * bankSize), SEEK_SET);
			}

			if (format != 3)
			{
				romData = (unsigned char*)malloc(bankSize);
				fread(romData, 1, bankSize, rom);
			}
			else if (format == 3)
			{
				romData = (unsigned char*)malloc(bankSize * 2);
				fread(romData, 1, (bankSize * 2), rom);
			}

			if (format != 3)
			{
				/*Try to search the bank for song table loader*/
				for (i = 0; i < bankSize; i++)
				{
					if ((!memcmp(&romData[i], MagicBytesGB, 4)) && foundTable != 1)
					{
						tablePtrLoc = bankAmt + i - 6;
						printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
						tableOffset = ReadLE16(&romData[tablePtrLoc - bankAmt]);

						if (tableOffset == 0x235E)
						{
							tableOffset = 0x4BCC;
						}
						else if (tableOffset == 0x4000)
						{
							smd2 = 1;
						}
						printf("Song table starts at 0x%04x...\n", tableOffset);
						foundTable = 1;
						format = 2;
					}
				}

			}


			if (foundTable == 0 && ReadLE16(&romData[0x00]) == 0x4040)
			{
				tableOffset = 0x4000;
				printf("Song table starts at 0x%04x...\n", tableOffset);
				smd2 = 1;
				foundTable = 1;
			}

			/*Try to search the bank for song table loader*/
			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&romData[i], MagicBytesNES, 9)) && foundTable != 1)
				{
					tablePtrLoc = bankAmt + i + 9;
					printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
					tableOffset = ReadLE16(&romData[tablePtrLoc - bankAmt]);

					bankAmt = 0x8000;
					printf("Song table starts at 0x%04x...\n", tableOffset);
					foundTable = 1;
					format = 1;
				}
			}

			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&romData[i], MagicBytesNES2, 4)) && foundTable != 1)
				{
					tablePtrLoc = bankAmt + i + 4;
					printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
					tableOffset = ReadLE16(&romData[tablePtrLoc - bankAmt]);

					bankAmt = 0x8000;
					printf("Song table starts at 0x%04x...\n", tableOffset);
					foundTable = 1;
					format = 1;
				}
			}

			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&romData[i], MagicBytesNES3, 6)) && foundTable != 1)
				{
					tablePtrLoc = bankAmt + i + 6;
					printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
					tableOffset = ReadLE16(&romData[tablePtrLoc - bankAmt]);

					bankAmt = 0x8000;
					printf("Song table starts at 0x%04x...\n", tableOffset);
					foundTable = 1;
					format = 1;
				}
			}

			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&romData[i], MagicBytesGG, 7)) && foundTable != 1)
				{
					tablePtrLoc = bankAmt + i + 7;
					printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
					tableOffset = ReadLE16(&romData[tablePtrLoc - bankAmt]);

					bankAmt = 0x4000;
					printf("Song table starts at 0x%04x...\n", tableOffset);
					foundTable = 1;
					format = 3;
				}
			}

			if (foundTable == 1)
			{
				if (format == 2)
				{
					i = tableOffset - bankAmt;
					songNum = 1;
					i += 2;
					while (ReadLE16(&romData[i]) > bankAmt && ReadLE16(&romData[i]) < (bankSize * 2))
					{
						songPtr = ReadLE16(&romData[i]);
						printf("Song %i: 0x%04X\n", songNum, songPtr);
						song2mid(songNum, songPtr);
						i += 2;
						songNum++;
					}
				}
				else if (format == 3)
				{
					i = tableOffset - bankAmt;
					songNum = 1;
					i += 2;
					while (ReadLE16(&romData[i]) > bankAmt && ReadLE16(&romData[i]) < (bankSize * 2))
					{
						songPtr = ReadLE16(&romData[i]);
						printf("Song %i: 0x%04X\n", songNum, songPtr);
						song2mid(songNum, songPtr);
						i += 2;
						songNum++;
					}
				}
				else
				{
					i = tableOffset - bankAmt;
					songNum = 1;
					i += 2;
					while (ReadLE16(&romData[i]) > bankAmt && ReadLE16(&romData[i]) < 0xC000 && songNum < 117)
					{
						songPtr = ReadLE16(&romData[i]);
						printf("Song %i: 0x%04X\n", songNum, songPtr);
						song2mid(songNum, songPtr);
						i += 2;
						songNum++;
					}
				}

			}
			else
			{
				printf("ERROR: Magic bytes not found!\n");
				exit(-1);
			}

			free(romData);
			fclose(rom);
			printf("The operation was successfully completed!\n");
		}
	}
}

/*Convert the song data to TXT*/
void song2mid(int songNum, long ptr)
{
	static const char* TRK_NAMES_GB[4] = { "Square 1", "Square 2", "Wave", "Noise" };
	static const char* TRK_NAMES_NES[5] = { "Square 1", "Square 2", "Triangle", "Noise", "PCM" };
	int activeChan[4];
	int maskArray[4];
	unsigned char mask = 0;
	long romPos = 0;
	long seqPos = 0;
	int curTrack = 0;
	int trackCnt = 4;
	int ticks = 120;
	int tempo = 150;
	int k = 0;
	int seqEnd = 0;
	int curNote = 0;
	unsigned int curNoteLen = 0;
	int chanSpeed = 0;
	int octave = 0;
	int octCtrl = 0;
	int octCtrl2 = 0;
	int transpose = 0;
	long jumpPos = 0;
	long macro1Pos = 0;
	long macro1Ret = 0;
	long macro2Pos = 0;
	long macro2Ret = 0;
	int inMacro = 0;
	unsigned char command[8];
	unsigned char lowNibble = 0;
	unsigned char highNibble = 0;
	int bankAmt2 = 0xD900;
	long ramPos;
	int firstNote = 1;
	unsigned int midPos = 0;
	unsigned int ctrlMidPos = 0;
	long midTrackBase = 0;
	long ctrlMidTrackBase = 0;
	int valSize = 0;
	long trackSize = 0;
	int rest = 0;
	int tempByte = 0;
	int curDelay = 0;
	int ctrlDelay = 0;
	long tempPos = 0;
	int holdNote = 0;
	long startPos = 0;
	int repeat1 = 0;
	long repeat1Pt = 0;
	int repeat2 = 0;
	long repeat2Pt = 0;

	curInst = 0;
	curVol = 100;

	midPos = 0;
	ctrlMidPos = 0;

	midLength = 0x10000;
	midData = (unsigned char*)malloc(midLength);

	ctrlMidData = (unsigned char*)malloc(midLength);

	for (j = 0; j < midLength; j++)
	{
		midData[j] = 0;
		ctrlMidData[j] = 0;
	}

	sprintf(outfile, "song%d.mid", songNum);
	if ((mid = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file song%d.mid!\n", songNum);
		exit(2);
	}
	else
	{

		if (usePALTempo == 1)
		{
			tempo = 120;
		}
		else
		{
			tempo = 150;
		}
		/*Write MIDI header with "MThd"*/
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x4D546864);
		WriteBE32(&ctrlMidData[ctrlMidPos + 4], 0x00000006);
		ctrlMidPos += 8;

		WriteBE16(&ctrlMidData[ctrlMidPos], 0x0001);
		WriteBE16(&ctrlMidData[ctrlMidPos + 2], trackCnt + 1);
		WriteBE16(&ctrlMidData[ctrlMidPos + 4], ticks);
		ctrlMidPos += 6;
		/*Write initial MIDI information for "control" track*/
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x4D54726B);
		ctrlMidPos += 8;
		ctrlMidTrackBase = ctrlMidPos;

		/*Set channel name (blank)*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE16(&ctrlMidData[ctrlMidPos], 0xFF03);
		Write8B(&ctrlMidData[ctrlMidPos + 2], 0);
		ctrlMidPos += 2;

		/*Set initial tempo*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0xFF5103);
		ctrlMidPos += 4;

		WriteBE24(&ctrlMidData[ctrlMidPos], 60000000 / tempo);
		ctrlMidPos += 3;

		/*Set time signature*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE24(&ctrlMidData[ctrlMidPos], 0xFF5804);
		ctrlMidPos += 3;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x04021808);
		ctrlMidPos += 4;

		/*Set key signature*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE24(&ctrlMidData[ctrlMidPos], 0xFF5902);
		ctrlMidPos += 4;

		romPos = ptr - bankAmt;
		mask = romData[romPos];
		ramPos = romPos;

		/*Try to get active channels for sound effects*/
		maskArray[3] = mask >> 7 & 1;
		maskArray[2] = mask >> 6 & 1;
		maskArray[1] = mask >> 5 & 1;
		maskArray[0] = mask >> 4 & 1;

		/*Otherwise, it is music*/
		if (maskArray[0] == 0 && maskArray[1] == 0 && maskArray[2] == 0 && maskArray[3] == 0)
		{
			maskArray[3] = mask >> 3 & 1;
			maskArray[2] = mask >> 2 & 1;
			maskArray[1] = mask >> 1 & 1;
			maskArray[0] = mask & 1;
		}
		romPos++;

		for (curTrack = 0; curTrack < trackCnt; curTrack++)
		{
			firstNote = 1;
			holdNote = 0;
			chanSpeed = 1;
			/*Write MIDI chunk header with "MTrk"*/
			WriteBE32(&midData[midPos], 0x4D54726B);
			octave = 3;
			midPos += 8;
			midTrackBase = midPos;

			curDelay = 0;
			ctrlDelay = 0;
			seqEnd = 0;

			curNote = 0;
			curNoteLen = 0;
			transpose = 0;
			octCtrl = 0;
			octCtrl2 = 0;
			inMacro = 0;
			repeat1 = -1;
			repeat2 = -1;

			/*Add track header*/
			valSize = WriteDeltaTime(midData, midPos, 0);
			midPos += valSize;
			WriteBE16(&midData[midPos], 0xFF03);
			midPos += 2;
			if (format == 2 || format == 3)
			{
				Write8B(&midData[midPos], strlen(TRK_NAMES_GB[curTrack]));

			}
			else
			{
				Write8B(&midData[midPos], strlen(TRK_NAMES_NES[curTrack]));
			}
			midPos++;
			if (format == 2 || format == 3)
			{
				sprintf((char*)&midData[midPos], TRK_NAMES_GB[curTrack]);
				midPos += strlen(TRK_NAMES_GB[curTrack]);
			}
			else
			{
				sprintf((char*)&midData[midPos], TRK_NAMES_NES[curTrack]);
				midPos += strlen(TRK_NAMES_NES[curTrack]);
			}


			/*Calculate MIDI channel size*/
			trackSize = midPos - midTrackBase;
			WriteBE16(&midData[midTrackBase - 2], trackSize);

			if (maskArray[curTrack] != 0)
			{
				seqEnd = 0;
				seqPos = ReadLE16(&romData[romPos]);

				if (seqPos < 0xD900)
				{
					smd2 = 0;
					seqPos -= bankAmt;
				}
				else
				{
					smd2 = 1;
					base = ramPos;
					seqPos = seqPos - 0xD900 + ramPos;
				}
				romPos += 2;
			}
			else
			{
				seqEnd = 1;
			}

			if (curTrack == 3)
			{
				octCtrl = 36;
			}

			while (seqEnd == 0 && midPos < 48000 && ctrlDelay < 110000)
			{
				command[0] = romData[seqPos];
				command[1] = romData[seqPos + 1];
				command[2] = romData[seqPos + 2];

				/*Play note*/
				if (command[0] < 0xC0)
				{
					lowNibble = (command[0] >> 4);
					highNibble = (command[0] & 15) + 1;

					curNote = lowNibble + octCtrl + transpose + 12 + octCtrl2;
					curNoteLen = highNibble * chanSpeed * 5;

					if (curNote < 0)
					{
						curNote = 0;
					}
					tempPos = WriteNoteEvent(midData, midPos, curNote, curNoteLen, curDelay, firstNote, curTrack, curInst);
					firstNote = 0;
					midPos = tempPos;
					curDelay = 0;
					ctrlDelay += curNoteLen;
					seqPos++;
				}

				/*Rest*/
				else if (command[0] >= 0xC0 && command[0] <= 0xCF)
				{
					rest = command[0] - 0xBF;
					curDelay += (rest * chanSpeed * 5);
					ctrlDelay += (rest * chanSpeed * 5);
					seqPos++;
				}

				/*Set octave*/
				else if (command[0] >= 0xD0 && command[0] <= 0xD6)
				{
					switch (command[0])
					{
					case 0xD0:
						octCtrl = 0;
						break;
					case 0xD1:
						octCtrl = 12;
						break;
					case 0xD2:
						octCtrl = 24;
						break;
					case 0xD3:
						octCtrl = 36;
						break;
					case 0xD4:
						octCtrl = 48;
						break;
					case 0xD5:
						octCtrl = 60;
						break;
					case 0xD6:
						octCtrl = 72;
						break;
					default:
						octCtrl = 0;
						break;
					}
					seqPos++;
				}

				/*Raise octave*/
				else if (command[0] == 0xD7)
				{
					octCtrl += 12;
					seqPos++;
				}

				/*Lower octave*/
				else if (command[0] == 0xD8)
				{
					octCtrl -= 12;
					seqPos++;
				}

				/*Transpose channel*/
				else if (command[0] == 0xD9)
				{
					transpose = (signed char)command[1];
					seqPos += 2;
				}

				/*Increment octave control*/
				else if (command[0] == 0xDA)
				{
					octCtrl2 += (signed char)command[1];
					seqPos += 2;
				}

				/*Set tuning*/
				else if (command[0] == 0xDB)
				{
					seqPos += 2;
				}

				/*(Invalid)*/
				else if (command[0] >= 0xDC && command[0] <= 0xDE)
				{
					seqPos += 2;
				}

				/*Unknown command DF (used in Momotarou games)*/
				else if (command[0] == 0xDF)
				{
					seqPos += 2;
				}

				/*Set channel speed*/
				else if (command[0] == 0xE0)
				{
					chanSpeed = command[1];
					seqPos += 2;
				}

				/*Set channel effect*/
				else if (command[0] == 0xE1)
				{
					seqPos += 2;
				}

				/*Set duty*/
				else if (command[0] == 0xE2)
				{
					if (format == 1)
					{
						curInst = command[1];
						if (curInst >= 0x80)
						{
							curInst = 0;
						}
						firstNote = 1;
					}
					seqPos += 2;
				}

				/*Set panning*/
				else if (command[0] == 0xE3)
				{
					seqPos += 2;
				}

				/*Set vibrato*/
				else if (command[0] == 0xE4)
				{
					seqPos += 2;
				}

				/*Set distortion effect*/
				else if (command[0] == 0xE5)
				{
					seqPos += 2;
				}

				/*Set envelope/volume*/
				else if (command[0] == 0xE6)
				{
					seqPos += 2;
				}

				/*(Invalid)*/
				else if (command[0] == 0xE7)
				{
					seqPos += 2;
				}

				/*Next note is combined length of previous note*/
				else if (command[0] == 0xE8)
				{
					seqPos++;
				}

				/*Set envelope*/
				else if (command[0] == 0xE9)
				{
					seqPos += 2;
				}

				/*Set envelope (v2)?*/
				else if (command[0] == 0xEA)
				{
					seqPos += 2;
				}

				/*Set sweep*/
				else if (command[0] == 0xEB)
				{
					seqPos += 2;
				}

				/*Set note size?*/
				else if (command[0] == 0xEC)
				{
					seqPos += 2;
				}

				/*Set note size (v2)?*/
				else if (command[0] == 0xED)
				{
					seqPos += 2;
				}

				/*(Invalid)*/
				else if (command[0] == 0xEE || command[0] == 0xEF)
				{
					seqPos += 2;
				}

				/*Repeat the following section*/
				else if (command[0] == 0xF0)
				{
					if (repeat1 == -1)
					{
						repeat1 = command[1];
						repeat1Pt = seqPos + 2;
						seqPos += 2;
					}
					else
					{
						repeat2 = command[1];
						repeat2Pt = seqPos + 2;
						seqPos += 2;
					}
				}

				/*End of repeat section*/
				else if (command[0] == 0xF1)
				{
					if (repeat2 == -1)
					{
						if (repeat1 > 1)
						{
							seqPos = repeat1Pt;
							repeat1--;
						}
						else if (repeat1 <= 1)
						{
							repeat1 = -1;
							seqPos++;
						}
					}
					else if (repeat2 > -1)
					{
						if (repeat2 > 1)
						{
							seqPos = repeat2Pt;
							repeat2--;
						}
						else if (repeat2 <= 1)
						{
							repeat2 = -1;
							seqPos++;
						}
					}
				}

				/*Go to macro*/
				else if (command[0] == 0xF2)
				{
					if (inMacro == 0)
					{
						if (smd2 == 0)
						{
							macro1Pos = ReadLE16(&romData[seqPos + 1]) - bankAmt;
							macro1Ret = seqPos += 3;
							seqPos = macro1Pos;
							inMacro = 1;
						}
						else
						{
							macro1Pos = ReadLE16(&romData[seqPos + 1]);
							macro1Pos = macro1Pos - 0xD900 + ramPos;
							macro1Ret = seqPos += 3;
							seqPos = macro1Pos;
							inMacro = 1;
						}

					}
					else if (inMacro == 1)
					{
						if (smd2 == 0)
						{
							macro2Pos = ReadLE16(&romData[seqPos + 1]) - bankAmt;
							macro2Ret = seqPos += 3;
							seqPos = macro2Pos;
							inMacro = 2;
						}
						else
						{
							macro2Pos = ReadLE16(&romData[seqPos + 1]);
							macro2Pos = macro2Pos - 0xD900 + ramPos;
							macro2Ret = seqPos += 3;
							seqPos = macro2Pos;
							inMacro = 1;
						}

					}
					else
					{
						seqEnd = 1;
					}
				}

				/*Return from macro*/
				else if (command[0] == 0xF3)
				{
					if (inMacro == 1)
					{
						seqPos = macro1Ret;
						inMacro = 0;
					}
					else if (inMacro == 2)
					{
						seqPos = macro2Ret;
						inMacro = 1;
					}
					else
					{
						seqEnd = 1;
					}
				}

				/*(Invalid)*/
				else if (command[0] >= 0xF4 && command[0] <= 0xF7)
				{
					seqPos += 2;
				}

				/*Go to loop point*/
				else if (command[0] == 0xF8)
				{
					seqEnd = 1;
				}

				/*(Invalid)*/
				else if (command[0] >= 0xF9 && command[0] <= 0xFE)
				{
					seqPos += 2;
				}

				/*End of channel*/
				else if (command[0] == 0xFF)
				{
					seqEnd = 1;
				}
			}

			/*End of track*/
			WriteBE32(&midData[midPos], 0xFF2F00);
			midPos += 4;

			/*Calculate MIDI channel size*/
			trackSize = midPos - midTrackBase;
			WriteBE16(&midData[midTrackBase - 2], trackSize);
		}

		/*End of control track*/
		ctrlMidPos++;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0xFF2F00);
		ctrlMidPos += 4;

		/*Calculate MIDI channel size*/
		trackSize = ctrlMidPos - ctrlMidTrackBase;
		WriteBE16(&ctrlMidData[ctrlMidTrackBase - 2], trackSize);

		sprintf(outfile, "song%d.mid", songNum);
		fwrite(ctrlMidData, ctrlMidPos, 1, mid);
		fwrite(midData, midPos, 1, mid);
		free(midData);
		free(ctrlMidData);
		fclose(mid);
	}
}