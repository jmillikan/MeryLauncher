/*
 *  Copyright (C) 2008 Nuke (wiinuke@gmail.com)
 *
 *  this file is part of GeckoOS for USB Gecko
 *  http://www.usbgecko.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include <sys/unistd.h>
#include <ogc/ipc.h>
#include <malloc.h>
#include "fst.h"
#include "gecko.h"
#include "memory.h"
#include "patchcode.h"
#include "codehandler.h"
#include "codehandleronly.h"
#include "multidol.h"

#define FSTDIRTYPE 1
#define FSTFILETYPE 0
#define ENTRYSIZE 0xC

#define MAX_FILENAME_LEN	128

u8 *codelistend = NULL;
void *codelist = NULL;

u32 gameconfsize = 0;
u32 *gameconf = NULL;

u8 debuggerselect = 0;

extern const u32 viwiihooks[4];
extern const u32 kpadhooks[4];
extern const u32 joypadhooks[4];
extern const u32 gxdrawhooks[4];
extern const u32 gxflushhooks[4];
extern const u32 ossleepthreadhooks[4];
extern const u32 axnextframehooks[4];
extern const u32 wpadbuttonsdownhooks[4];
extern const u32 wpadbuttonsdown2hooks[4];

int app_gameconfig_load(const char *discid, u8 *tempgameconf, u32 tempgameconfsize)
{
	if (gameconf == NULL)
	{
		gameconf = (u32*) malloc(65536);
		if (gameconf == NULL)
			return -1;
	}
	u32 ret;
	s32 gameidmatch, maxgameidmatch = -1, maxgameidmatch2 = -1;
	u32 i, numnonascii, parsebufpos;
	u32 codeaddr, codeval, codeaddr2, codeval2, codeoffset;
	u32 temp, tempoffset = 0;
	char parsebuffer[18];

	// Remove non-ASCII characters
	numnonascii = 0;
	for (i = 0; i < tempgameconfsize; i++)
	{
		if (tempgameconf[i] < 9 || tempgameconf[i] > 126)
			numnonascii++;
		else
			tempgameconf[i - numnonascii] = tempgameconf[i];
	}
	tempgameconfsize -= numnonascii;

	*(tempgameconf + tempgameconfsize) = 0;
	//gameconf = (tempgameconf + tempgameconfsize) + (4 - (((u32) (tempgameconf + tempgameconfsize)) % 4));

	for (maxgameidmatch = 0; maxgameidmatch <= 6; maxgameidmatch++)
	{
		i = 0;
		while (i < tempgameconfsize)
		{
			maxgameidmatch2 = -1;
			while (maxgameidmatch != maxgameidmatch2)
			{
				while (i != tempgameconfsize && tempgameconf[i] != ':')
					i++;
				if (i == tempgameconfsize) break;
				while ((tempgameconf[i] != 10 && tempgameconf[i] != 13) && (i != 0))
					i--;
				if (i != 0) i++;
				parsebufpos = 0;
				gameidmatch = 0;
				while (tempgameconf[i] != ':')
				{
					if (tempgameconf[i] == '?')
					{
						parsebuffer[parsebufpos] = discid[parsebufpos];
						parsebufpos++;
						gameidmatch--;
						i++;
					}
					else if (tempgameconf[i] != 0 && tempgameconf[i] != ' ')
						parsebuffer[parsebufpos++] = tempgameconf[i++];
					else if (tempgameconf[i] == ' ')
						break;
					else i++;
					if (parsebufpos == 8) break;
				}
				parsebuffer[parsebufpos] = 0;
				if (strncasecmp("DEFAULT", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 7)
				{
					gameidmatch = 0;
					goto idmatch;
				}
				if (strncasecmp(discid, parsebuffer, strlen(parsebuffer)) == 0)
				{
					gameidmatch += strlen(parsebuffer);
					idmatch: if (gameidmatch > maxgameidmatch2)
					{
						maxgameidmatch2 = gameidmatch;
					}
				}
				while ((i != tempgameconfsize) && (tempgameconf[i] != 10 && tempgameconf[i] != 13))
					i++;
			}
			while (i != tempgameconfsize && tempgameconf[i] != ':')
			{
				parsebufpos = 0;
				while ((i != tempgameconfsize) && (tempgameconf[i] != 10 && tempgameconf[i] != 13))
				{
					if (tempgameconf[i] != 0 && tempgameconf[i] != ' ' && tempgameconf[i] != '(' && tempgameconf[i]
							!= ':')
						parsebuffer[parsebufpos++] = tempgameconf[i++];
					else if (tempgameconf[i] == ' ' || tempgameconf[i] == '(' || tempgameconf[i] == ':')
						break;
					else i++;
					if (parsebufpos == 17) break;
				}
				parsebuffer[parsebufpos] = 0;
				//if (!autobootcheck)
				{
					if (strncasecmp("codeliststart", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer)
							== 13)
					{
						sscanf((char *) (tempgameconf + i), " = %x", (unsigned int *) &codelist);
					}
					if (strncasecmp("codelistend", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 11)
					{
						sscanf((char *) (tempgameconf + i), " = %x", (unsigned int *) &codelistend);
					}
					if (strncasecmp("poke", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 4)
					{
						ret = sscanf((char *) tempgameconf + i, "( %x , %x", &codeaddr, &codeval);
						if (ret == 2)
						{
							*(gameconf + (gameconfsize / 4)) = 0;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = 0;
							gameconfsize += 8;
							*(gameconf + (gameconfsize / 4)) = codeaddr;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeval;
							gameconfsize += 4;
							DCFlushRange((void *) (gameconf + (gameconfsize / 4) - 5), 20);
						}
					}
					if (strncasecmp("pokeifequal", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 11)
					{
						ret = sscanf((char *) (tempgameconf + i), "( %x , %x , %x , %x", &codeaddr, &codeval,
								&codeaddr2, &codeval2);
						if (ret == 4)
						{
							*(gameconf + (gameconfsize / 4)) = 0;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeaddr;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeval;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeaddr2;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeval2;
							gameconfsize += 4;
							DCFlushRange((void *) (gameconf + (gameconfsize / 4) - 5), 20);
						}
					}
					if (strncasecmp("searchandpoke", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer)
							== 13)
					{
						ret = sscanf((char *) (tempgameconf + i), "( %x%n", &codeval, &tempoffset);
						if (ret == 1)
						{
							gameconfsize += 4;
							temp = 0;
							while (ret == 1)
							{
								*(gameconf + (gameconfsize / 4)) = codeval;
								gameconfsize += 4;
								temp++;
								i += tempoffset;
								ret = sscanf((char *) (tempgameconf + i), " %x%n", &codeval, &tempoffset);
							}
							*(gameconf + (gameconfsize / 4) - temp - 1) = temp;
							ret = sscanf((char *) (tempgameconf + i), " , %x , %x , %x , %x", &codeaddr, &codeaddr2,
									&codeoffset, &codeval2);
							if (ret == 4)
							{
								*(gameconf + (gameconfsize / 4)) = codeaddr;
								gameconfsize += 4;
								*(gameconf + (gameconfsize / 4)) = codeaddr2;
								gameconfsize += 4;
								*(gameconf + (gameconfsize / 4)) = codeoffset;
								gameconfsize += 4;
								*(gameconf + (gameconfsize / 4)) = codeval2;
								gameconfsize += 4;
								DCFlushRange((void *) (gameconf + (gameconfsize / 4) - temp - 5), temp * 4 + 20);
							}
							else gameconfsize -= temp * 4 + 4;
						}

					}
				}
				if (tempgameconf[i] != ':')
				{
					while ((i != tempgameconfsize) && (tempgameconf[i] != 10 && tempgameconf[i] != 13))
						i++;
					if (i != tempgameconfsize) i++;
				}
			}
			if (i != tempgameconfsize) while ((tempgameconf[i] != 10 && tempgameconf[i] != 13) && (i != 0))
				i--;
		}
	}
	return 0;
}

void app_gameconfig_set(u32 *gameconfig, u32 tempgameconfsize)
{
	if(gameconfig == NULL)
		return;
	gameconfsize = tempgameconfsize;
	gameconf = malloc(gameconfsize); //internal copy
	memcpy(gameconf, gameconfig, gameconfsize);
}

u8 *code_buf = NULL;
u32 code_size = 0;

void ocarina_set_codes(void *list, u8 *listend, u8 *cheats, u32 cheatSize)
{
	if(codelist == NULL)
		codelist = list;
	if(codelistend == NULL)
		codelistend = listend;
	if(cheatSize <= 0 || cheats == NULL)
	{
		gprintf("Ocarina: No codes found\n");
		code_buf = NULL;
		code_size = 0;
		return;
	}
	if (cheatSize > (u32)codelistend - (u32)codelist)
	{
		gprintf("Ocarina: Too many codes found.\n");
		code_buf = NULL;
		code_size = 0;
		return;
	}
	code_size = cheatSize;
	code_buf = malloc(code_size); //internal copy
	memcpy(code_buf, cheats, code_size);
	gprintf("Ocarina: Codes found.\n");
}

void app_pokevalues()
{
	u32 i, *codeaddr, *codeaddr2, *addrfound = NULL;

	if (gameconfsize != 0)
	{
		for (i = 0; i < gameconfsize/4; i++)
		{
			if (*(gameconf + i) == 0)
			{
				if (((u32 *) (*(gameconf + i + 1))) == NULL ||
					*((u32 *) (*(gameconf + i + 1))) == *(gameconf + i + 2))
				{
					*((u32 *) (*(gameconf + i + 3))) = *(gameconf + i + 4);
					DCFlushRange((void *) *(gameconf + i + 3), 4);
				}
				i += 4;
			}
			else
			{
				codeaddr = (u32 *)*(gameconf + i + *(gameconf + i) + 1);
				codeaddr2 = (u32 *)*(gameconf + i + *(gameconf + i) + 2);
				if (codeaddr == 0 && addrfound != NULL)
					codeaddr = addrfound;
				else if (codeaddr == 0 && codeaddr2 != 0)
					codeaddr = (u32 *) ((((u32) codeaddr2) >> 28) << 28);
				else if (codeaddr == 0 && codeaddr2 == 0)
				{
					i += *(gameconf + i) + 4;
					continue;
				}
				if (codeaddr2 == 0)
					codeaddr2 = codeaddr + *(gameconf + i);
				addrfound = NULL;
				while (codeaddr <= (codeaddr2 - *(gameconf + i)))
				{
					if (memcmp(codeaddr, gameconf + i + 1, (*(gameconf + i)) * 4) == 0)
					{
						*(codeaddr + ((*(gameconf + i + *(gameconf + i) + 3)) / 4)) = *(gameconf + i + *(gameconf + i) + 4);
						if (addrfound == NULL) addrfound = codeaddr;
					}
					codeaddr++;
				}
				i += *(gameconf + i) + 4;
			}
		}
	}
}

void load_handler()
{
	if(debuggerselect == 0x01)
	{
		gprintf("Ocarina: Debugger selected.\n");
		memcpy((void*)0x80001800, codehandler, codehandler_size);
		if(code_size > 0 && code_buf)
		{
			gprintf("Ocarina: Codes found.\n");
			memcpy((void*)0x80001CDE, &codelist, 2);
			memcpy((void*)0x80001CE2, ((u8*) &codelist) + 2, 2);
			memcpy((void*)0x80001F5A, &codelist, 2);
			memcpy((void*)0x80001F5E, ((u8*) &codelist) + 2, 2);
		}
		else
		{
			gprintf("Ocarina: No Codes found.\n");
			*(u32*)0x80002774 = 1; //pause for debugger
		}
		DCFlushRange((void*)0x80001800, codehandler_size);
		ICInvalidateRange((void*)0x80001800, codehandler_size);
	}
	else
	{
		gprintf("Ocarina: No Debugger selected.\n");
		memcpy((void*)0x80001800, codehandleronly, codehandleronly_size);
		if(code_size > 0 && code_buf)
		{
			gprintf("Ocarina: Codes found.\n");
			memcpy((void*)0x80001906, &codelist, 2);
			memcpy((void*)0x8000190A, ((u8*) &codelist) + 2, 2);
		}
		else
			gprintf("Ocarina: No Codes found.\n");
		DCFlushRange((void*)0x80001800, codehandleronly_size);
		ICInvalidateRange((void*)0x80001800, codehandleronly_size);
	}

	// Load multidol handler
	memcpy((void*)0x80001000, multidol, multidol_size);
	DCFlushRange((void*)0x80001000, multidol_size);
	ICInvalidateRange((void*)0x80001000, multidol_size);
	switch(hooktype)
	{
		case 0x01:
			memcpy((void*)0x8000119C,viwiihooks,12);
			memcpy((void*)0x80001198,viwiihooks+3,4);
			break;
		case 0x02:
			memcpy((void*)0x8000119C,kpadhooks,12);
			memcpy((void*)0x80001198,kpadhooks+3,4);
			break;
		case 0x03:
			memcpy((void*)0x8000119C,joypadhooks,12);
			memcpy((void*)0x80001198,joypadhooks+3,4);
			break;
		case 0x04:
			memcpy((void*)0x8000119C,gxdrawhooks,12);
			memcpy((void*)0x80001198,gxdrawhooks+3,4);
			break;
		case 0x05:
			memcpy((void*)0x8000119C,gxflushhooks,12);
			memcpy((void*)0x80001198,gxflushhooks+3,4);
			break;
		case 0x06:
			memcpy((void*)0x8000119C,ossleepthreadhooks,12);
			memcpy((void*)0x80001198,ossleepthreadhooks+3,4);
			break;
		case 0x07:
			memcpy((void*)0x8000119C,axnextframehooks,12);
			memcpy((void*)0x80001198,axnextframehooks+3,4);
			break;
		case 0x08:
			//if (customhooksize == 16)
			//{
			//	memcpy((void*)0x8000119C,customhook,12);
			//	memcpy((void*)0x80001198,customhook+3,4);
			//}
			break;
		case 0x09:
			//memcpy((void*)0x8000119C,wpadbuttonsdownhooks,12);
			//memcpy((void*)0x80001198,wpadbuttonsdownhooks+3,4);
			break;
		case 0x0A:
			//memcpy((void*)0x8000119C,wpadbuttonsdown2hooks,12);
			//memcpy((void*)0x80001198,wpadbuttonsdown2hooks+3,4);
			break;
	}
	DCFlushRange((void*)0x80001198,16);
}

int ocarina_do_code()
{
	//if (!code_buf) return 0;  // Need the handler loaded for hooking other than cheats!
	load_handler();

	if(codelist)
		memset(codelist, 0, (u32)codelistend - (u32)codelist);

	//Copy the codes
	if(code_size > 0 && code_buf)
	{
		memcpy(codelist, code_buf, code_size);
		DCFlushRange(codelist, (u32)codelistend - (u32)codelist);
		free(code_buf);
		code_buf = NULL;
	}

	// TODO What's this???
	// enable flag
	//*(vu8*)0x80001807 = 0x01;

	//This needs to be done after loading the .dol into memory
	app_pokevalues();

	return 1;
}
