/******************************************************************************
 * File: gs.h
 * Created: 2016-07-14
 * Last Updated: 2016-08-09
 * Creator: Aaron Oman (a.k.a GrooveStomp)
 * Notice: (C) Copyright 2016 by Aaron Oman
 *-----------------------------------------------------------------------------
 *
 * Standard library for personal use. Heavily influenced by Sean Barrett's stb.
 *
 ******************************************************************************/
#define GS_VERSION 0.1.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* memset */
#include <stdarg.h> /* va_list */

#define GSArraySize(Array) (sizeof((Array)) / sizeof((Array)[0]))

/* Full credit: http://stackoverflow.com/a/400970 */
#define GSArrayForEach(Item, Array) \
        for(int Keep##__LINE__ = 1, \
                Count##__LINE__ = 0, \
                Index = 0, \
                Size##__LINE__ = sizeof((Array)) / sizeof(*(Array)); \
            Keep##__LINE__ && Count##__LINE__ != Size##__LINE__; \
            Keep##__LINE__ = !Keep##__LINE__, Count##__LINE__++) \
                for(Item = (Array) + Count##__LINE__; Keep##__LINE__; Keep##__LINE__ = !Keep##__LINE__, Index++)

#define GSMax(A, B) ((A) < (B) ? (B) : (A))
#define GSMin(A, B) ((A) < (B) ? (A) : (B))

void
GSAbortWithMessage(char *FormatString, ...)
{
        va_list Args;
        va_start(Args, FormatString);
        fprintf(stderr, FormatString, Args);
        exit(EXIT_FAILURE);
}

/******************************************************************************
 * Boolean Definitions
 ******************************************************************************/

typedef int gs_bool;
#ifndef false
#define false 0
#define true !false
#endif

/******************************************************************************
 * Character Definitions
 *-----------------------------------------------------------------------------
 * Functions to interact with C's basic ASCII char type.
 ******************************************************************************/

gs_bool
GSCharIsEndOfStream(char C)
{
	return(C == '\0');
}

gs_bool
GSCharIsEndOfLine(char C)
{
	return((C == '\n') ||
	       (C == '\r'));
}

gs_bool
GSCharIsWhitespace(char C)
{
	return((C == ' ') ||
	       (C == '\t') ||
	       (C == '\v') ||
	       (C == '\f') ||
	       GSCharIsEndOfLine(C));
}

gs_bool
GSCharIsOctal(char C)
{
	gs_bool Result = (C >= '0' && C <= '7');
	return(Result);
}

gs_bool
GSCharIsDecimal(char C)
{
	gs_bool Result = (C >= '0' && C <= '9');
	return(Result);
}

gs_bool
GSCharIsHexadecimal(char C)
{
	gs_bool Result = ((C >= '0' && C <= '9') ||
		       (C >= 'a' && C <= 'f') ||
		       (C >= 'A' && C <= 'F'));
	return(Result);
}

gs_bool
GSCharIsAlphabetical(char C)
{
	gs_bool Result = ((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z'));
	return(Result);
}

gs_bool
GSCharIsAlphanumeric(char C)
{
        gs_bool Result = GSCharIsAlphabetical(C) || GSCharIsDecimal(C);
        return(Result);
}

/******************************************************************************
 * String Definitions
 *-----------------------------------------------------------------------------
 * C string type. That is, ASCII characters with terminating NULL.
 ******************************************************************************/

gs_bool
GSStringIsEqual(char *LeftString, char *RightString, int MaxNumToMatch)
{
	int NumMatched = 0;

	while(NumMatched < MaxNumToMatch)
	{
		if(*LeftString == *RightString)
		{
			LeftString++;
			RightString++;
			NumMatched++;
		}
		else
		{
			return(false);
		}
	}

	return(true);
}

size_t
GSStringLength(char *String)
{
	char *P = String;
	while(*P != '\0') P++;
	return(P - String);
}

gs_bool
GSStringCopyWithNull(char *Source, char *Dest, int Max)
{
        if(Source == NULL || Dest == NULL)
        {
                return(false);
        }

        int I = 0;
        for(; Source[I] != '\0' && I < Max; I++)
        {
                Dest[I] = Source[I];
        }
        Dest[I] = '\0';

        return(true);
}

gs_bool
GSStringCopy(char *Source, char *Dest, int Max)
{
        if(Source == NULL || Dest == NULL)
        {
                return(false);
        }

        for(int I = 0; Source[I] != '\0' && I < Max; I++)
        {
                Dest[I] = Source[I];
        }

        return(true);
}

unsigned int /* Returns number of bytes copied. */
GSStringCopyWithoutSurroundingWhitespace(char *Source, char *Dest, unsigned int MaxLength)
{
        int FirstChar, LastChar;
        for(FirstChar = 0; GSCharIsWhitespace(Source[FirstChar]); FirstChar++);

        int StringLength = GSMin(MaxLength, GSStringLength(Source));
        for(LastChar = StringLength - 1; GSCharIsWhitespace(Source[LastChar]); LastChar--);

        for(int D=0, S=FirstChar; S<=LastChar; D++, S++)
        {
                Dest[D] = Source[S];
        }

        int Count = (LastChar - FirstChar + 1);

        return(Count);
}

unsigned int /* Returns number of bytes copied. */
GSStringCopyWithoutSurroundingWhitespaceWithNull(char *Source, char *Dest, unsigned int MaxLength)
{
        int FirstChar, LastChar;
        for(FirstChar = 0; GSCharIsWhitespace(Source[FirstChar]); FirstChar++);

        int StringLength = GSMin(MaxLength, GSStringLength(Source));
        for(LastChar = StringLength - 1; GSCharIsWhitespace(Source[LastChar]); LastChar--);

        for(int I=FirstChar; I<=LastChar; I++)
        {
                Dest[I] = Source[I];
        }

        int Count = (LastChar - FirstChar + 1);
        Dest[Count] = '\0';

        return(Count);
}

/******************************************************************************
 * Hash Map
 *-----------------------------------------------------------------------------
 *
 * Usage:
 *     char *Value = "value";
 *     char *Result = NULL;
 *     gs_bool Found = false;
 *     gs_hash_map Map = GSHashMapCreate();
 *     GSHashMapAdd(&Map, "key", (void *)Value);
 *     Found = GSHashMapGet(&Map, "key", (void *)Result);
 *     if(Found) printf("Result: %s\n", Result);
 *
 ******************************************************************************/

#define GSHashMapInitialSize 50

typedef struct gs_hash_map {
        int Count;
        int Size;
        int *Keys;
        void *(*Values);
} gs_hash_map;

int
__GSHashMapCompute(char *Key)
{
        /*
          sdbm hash function: http://stackoverflow.com/a/14409947
        */
        int HashAddress = 0;
        for(int Counter = 0; Key[Counter] != '\0'; Counter++)
        {
                HashAddress = Key[Counter] +
                        (HashAddress << 6) +
                        (HashAddress << 16) -
                        HashAddress;
        }
        return(HashAddress);
}

size_t
GSHashMapSpaceRequired(int NumKeys)
{
        if(NumKeys == -1)
        {
                NumKeys = GSHashMapInitialSize;
        }

        int SizeOfKeys = sizeof(int) * NumKeys;
        int SizeOfValues = sizeof(void *) * NumKeys;
        int SizeToAlloc = SizeOfKeys + SizeOfValues + sizeof(gs_hash_map);
        return(SizeToAlloc);
}

gs_hash_map *
__GSHashMapAlloc(int NumKeys, void *Memory)
{
        gs_hash_map *NewHash;

        if(NumKeys == -1)
        {
                NumKeys = GSHashMapInitialSize;
        }

        int SizeToAlloc = GSHashMapSpaceRequired(NumKeys);
        int SizeOfKeys = sizeof(int) * NumKeys;

        if(Memory == NULL)
        {
                NewHash = (gs_hash_map*)calloc(1, SizeToAlloc);
        }
        else
        {
                NewHash = (gs_hash_map*)Memory;
                memset((void *)NewHash, 0, sizeof(SizeToAlloc));
        }

        NewHash->Keys = (int *)((char *)NewHash + sizeof(gs_hash_map));
        NewHash->Values = (void *)((char *)NewHash + sizeof(gs_hash_map) + SizeOfKeys);
        NewHash->Size = NumKeys;

        return(NewHash);
}

/*
  NumKeys: Number of initial key/value pairs expected. Specify -1 to use default.
  Memory:  Memory buffer to place HashMap into. Specify NULL to use malloc.
*/
gs_hash_map *
GSHashMapCreate(int NumKeys, void *Memory) {
        gs_hash_map *NewHash = __GSHashMapAlloc(NumKeys, Memory);
        return(NewHash);
}

gs_bool
GSHashMapAdd(gs_hash_map *Hash, char *Key, void *Value) {
        if(Hash->Size == Hash->Count) return false;

        int IntKey = __GSHashMapCompute(Key);
        Hash->Keys[Hash->Count] = IntKey;
        Hash->Values[Hash->Count++] = Value;
        return(true);
}

gs_bool
GSHashMapGet(gs_hash_map *Hash, char *Key, void **Value) {
        int IntKey = __GSHashMapCompute(Key);
        for(int i = 0; i < Hash->Count; ++i)
        {
                if(Hash->Keys[i] == IntKey)
                {
                        *Value = Hash->Values[i];
                        return true;
                }
        }
        return(false);
}

/******************************************************************************
 * Arg Parsing
 ******************************************************************************/

gs_bool
GSArgIsPresent(int Count, char **Args, char *Wanted)
{
        int StringLength = GSStringLength(Wanted);
        for(int I=0; I<Count; I++)
        {
                if(GSStringIsEqual(Wanted, Args[I], StringLength))
                {
                        return(true);
                }
        }
        return(false);
}

int /* Returns -1 if Arg not found. */
GSArgIndex(int Count, char **Args, char *Wanted)
{
        int StringLength = GSStringLength(Wanted);
        for(int I=0; I<Count; I++)
        {
                if(GSStringIsEqual(Wanted, Args[I], StringLength))
                {
                        return(I);
                }
        }
        return(-1);
}

char * /* Returns NULL if Index is invalid. */
GSArgAtIndex(int Count, char **Args, int Index)
{
        if((Index < 0) ||
           (Index > (Count - 1)))
                return(NULL);
        else
                return(Args[Index]);
}

char * /* Returns NULL if Marker is not found or no trailing arg. */
GSArgAfter(int Count, char **Args, char *Marker)
{
        int Index = GSArgIndex(Count, Args, Marker);
        if(Index < 0) return(NULL);

        char *Arg = GSArgAtIndex(Count, Args, Index + 1);
        return(Arg);
}

gs_bool
GSArgHelpWanted(int Count, char **Args)
{
        if(GSArgIsPresent(Count, Args, "-h") ||
           GSArgIsPresent(Count, Args, "--help"))
                return(true);
        else
                return(false);
}
