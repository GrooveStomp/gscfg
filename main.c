/******************************************************************************
 * File: main.c
 * Created: 2016-08-09
 * Last Updated: 2016-08-09
 * Creator: Aaron Oman (a.k.a GrooveStomp)
 * Notice: (C) Copyright 2016 by Aaron Oman
 *-----------------------------------------------------------------------------
 *
 * Static config generator.
 * Transforms a human-readable config file into a statically defined C struct
 * and initializer.
 *
 ******************************************************************************/
#include <alloca.h>
#include <stdio.h>
#include <stdlib.h> /* EXIT_SUCCESS */
#include <libgen.h> /* POSIX basename */
#include "gs.h"

const int MaxStringLength = 255;

typedef struct buffer
{
        char *Start;
        char *Cursor;
        size_t Capacity;
        size_t Length;
} buffer;

void
BufferResetCursor(buffer *Buffer)
{
        Buffer->Cursor = Buffer->Start;
}

gs_bool
BufferIsEOF(buffer *Buffer)
{
        int Size = Buffer->Cursor - Buffer->Start;
        gs_bool Result = Size >= Buffer->Length;
        return(Result);
}

buffer *
BufferSet(buffer *Buffer, char *Start, size_t Length, size_t Capacity)
{
        Buffer->Start = Start;
        Buffer->Cursor = Start;
        Buffer->Length = Length;
        Buffer->Capacity = Capacity;
        return(Buffer);
}

void
BufferAdvanceToNextLine(buffer *Buffer)
{
        while(true)
        {
                if(Buffer->Cursor[0] == '\n' ||
                   Buffer->Cursor[0] == '\0')
                {
                        break;
                }
                Buffer->Cursor++;
        }
        Buffer->Cursor++;
}

size_t
FileSize(char *FileName)
{
        size_t FileSize = 0;
        FILE *File = fopen(FileName, "r");
        if(File != NULL)
        {
                fseek(File, 0, SEEK_END);
                FileSize = ftell(File);
                fclose(File);
        }
        return(FileSize + 1);
}

gs_bool
CopyFileIntoBuffer(char *FileName, buffer *Buffer)
{
        FILE *File = fopen(FileName, "r");
        if(File == NULL) return(false);

        fseek(File, 0, SEEK_END);
        size_t FileSize = ftell(File);
        int Remaining = (Buffer->Start + Buffer->Capacity) - Buffer->Cursor;
        if(FileSize > Remaining) return(false);

        fseek(File, 0, SEEK_SET);
        fread(Buffer->Cursor, 1, FileSize, File);
        Buffer->Length += FileSize;
        Buffer->Cursor += FileSize;
        *(Buffer->Cursor) = '\0';

        return(true);
}

void
GenerateSourceFile(buffer *Buffer, char *ConfigFileBaseName, char *StructName)
{
        char *Key;
        char *Value;
        char Temp[MaxStringLength];

        char *StructDefineFilename = "_struct_define.c";
        FILE *StructDefine = fopen(StructDefineFilename, "w");
        fprintf(StructDefine, "typedef struct %s\n", StructName);
        fprintf(StructDefine, "{\n");

        char *StructInitFilename = "_struct_init.c";
        FILE *StructInit = fopen(StructInitFilename, "w");
        fprintf(StructInit, "void\n");
        fprintf(StructInit, "%sInit(%s *Self)\n", StructName, StructName);
        fprintf(StructInit, "{\n");

        while(true)
        {
                if(BufferIsEOF(Buffer)) break;

                /* NOTE(AARON): sscanf doesn't work if doing: sscanf("key:value", %s:%s, Key, Value); */
                /*              use strtok(...) instead. */
                /* TODO(AARON): Breaks on URL value because of ':' */
                char *Colon = strchr(Buffer->Cursor, ':');
                char *Newline = strchr(Buffer->Cursor, '\n');
                if(Colon == NULL || Newline < Colon)
                {
                        BufferAdvanceToNextLine(Buffer);
                        continue;
                }

                if(Newline != NULL)
                {
                        int Length = Newline - Buffer->Cursor;
                        char *StringDup = (char *)alloca(Length + 1);
                        GSStringCopyWithNull(Buffer->Cursor, StringDup, Length);
                        Key = strtok(StringDup, ":\n");
                        Value = strtok(NULL, "\n");

                        unsigned int StringLength;
                        memset(Temp, 0, MaxStringLength);
                        StringLength = GSMin(GSStringLength(Key), MaxStringLength);
                        StringLength = GSStringCopyWithoutSurroundingWhitespace(Key, Temp, StringLength);
                        GSStringCopy(Temp, Key, StringLength);

                        fprintf(StructDefine, "        char *%s;\n", Key);

                        if(Value != NULL)
                        {
                                memset(Temp, 0, MaxStringLength);
                                StringLength = GSMin(GSStringLength(Value), MaxStringLength);
                                StringLength = GSStringCopyWithoutSurroundingWhitespace(Value, Temp, StringLength);
                                GSStringCopyWithNull(Temp, Value, StringLength);

                                fprintf(StructInit,   "        Self->%s = \"%s\";\n", Key, Value);
                        }
                }
                BufferAdvanceToNextLine(Buffer);
        }

        fprintf(StructDefine, "} %s;\n", StructName);
        fprintf(StructInit, "}\n");

        fclose(StructDefine);
        fclose(StructInit);

        int AllocSize = FileSize(StructDefineFilename) + FileSize(StructInitFilename);
        buffer OutputBuffer;
        BufferSet(&OutputBuffer, (char *)alloca(AllocSize), 0, AllocSize);

        if(!CopyFileIntoBuffer(StructDefineFilename, &OutputBuffer))
                GSAbortWithMessage("Couldn't copy %s into memory\n", StructDefineFilename);

        if(!CopyFileIntoBuffer(StructInitFilename, &OutputBuffer))
                GSAbortWithMessage("Couldn't copy %s into memory\n", StructInitFilename);

        memset(Temp, 0, MaxStringLength);
        sprintf(Temp, "%s.c", ConfigFileBaseName);
        FILE *Out = fopen(Temp, "w");
        fwrite(OutputBuffer.Start, 1, OutputBuffer.Length, Out);
        fclose(Out);

        remove(StructDefineFilename);
        remove(StructInitFilename);
}

/******************************************************************************
 * Main
 ******************************************************************************/

void
Usage(char *ProgramName)
{
        printf("Usage: %s config-file [options]\n", basename(ProgramName));
        puts("");
        puts("Generates a c file with the same basename as config-file.");
        puts("This file declares a C struct that matches the structures of the config file.");
        puts("");
        puts("config-file: name of config file in current directory");
        puts("");
        puts("Options:");
        puts("\t--struct-name: Name of generated C struct. Defaults to config-file basename.");
        exit(EXIT_SUCCESS);
}

int
main(int ArgCount, char **Args)
{
        if(GSArgHelpWanted(ArgCount, Args) || ArgCount == 1) Usage(Args[0]);

        char ConfigCFile[MaxStringLength];
        char *ConfigFile = GSArgAtIndex(ArgCount, Args, 1);
        int StringLength = GSStringLength(ConfigFile);
        char *ExtensionStart = strchr(ConfigFile, '.');
        if(ExtensionStart != NULL)
        {
                StringLength = ExtensionStart - ConfigFile;
        }
        sprintf(ConfigCFile, "%.*s", StringLength, ConfigFile);

        char *StructName;
        if(GSArgIsPresent(ArgCount, Args, "--struct-name"))
        {
                StructName = GSArgAfter(ArgCount, Args, "--struct-name");
        }
        else
        {
                StructName = (char *)alloca(MaxStringLength);
                sprintf(StructName, "%.*s", StringLength, ConfigFile);
        }

        size_t AllocSize = FileSize(ConfigFile);
        buffer Buffer;
        BufferSet(&Buffer, (char *)alloca(AllocSize), 0, AllocSize);

        if(!CopyFileIntoBuffer(ConfigFile, &Buffer))
                GSAbortWithMessage("Couldn't copy %s into memory\n", ConfigFile);
        BufferResetCursor(&Buffer);

        GenerateSourceFile(&Buffer, ConfigCFile, StructName);

        return(EXIT_SUCCESS);
}
