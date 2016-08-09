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

void
GenerateSourceFile(gs_buffer *Buffer, char *ConfigFileBaseName, char *StructName)
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
                if(GSBufferIsEOF(Buffer)) break;

                /* NOTE(AARON): sscanf doesn't work if doing: sscanf("key:value", %s:%s, Key, Value); */
                /*              use strtok(...) instead. */
                /* TODO(AARON): Breaks on URL value because of ':' */
                char *Colon = strchr(Buffer->Cursor, ':');
                char *Newline = strchr(Buffer->Cursor, '\n');
                if(Colon == NULL || Newline < Colon)
                {
                        GSBufferNextLine(Buffer);
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
                GSBufferNextLine(Buffer);
        }

        fprintf(StructDefine, "} %s;\n", StructName);
        fprintf(StructInit, "}\n");

        fclose(StructDefine);
        fclose(StructInit);

        int AllocSize = GSFileSize(StructDefineFilename) + GSFileSize(StructInitFilename);
        gs_buffer OutputBuffer;
        GSBufferInit(&OutputBuffer, (char *)alloca(AllocSize), AllocSize);

        if(!GSFileCopyToBuffer(StructDefineFilename, &OutputBuffer))
                GSAbortWithMessage("Couldn't copy %s into memory\n", StructDefineFilename);

        if(!GSFileCopyToBuffer(StructInitFilename, &OutputBuffer))
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
main(int ArgCount, char **Arguments)
{
        gs_args Args;
        GSArgInit(&Args, ArgCount, Arguments);
        if(GSArgHelpWanted(&Args) || ArgCount == 1) Usage(GSArgProgramName(&Args));

        char ConfigCFile[MaxStringLength];
        char *ConfigFile = GSArgAtIndex(&Args, 1);
        int StringLength = GSStringLength(ConfigFile);
        char *ExtensionStart = strchr(ConfigFile, '.');
        if(ExtensionStart != NULL)
        {
                StringLength = ExtensionStart - ConfigFile;
        }
        sprintf(ConfigCFile, "%.*s", StringLength, ConfigFile);

        char *StructName;
        if(GSArgIsPresent(&Args, "--struct-name"))
        {
                StructName = GSArgAfter(&Args, "--struct-name");
        }
        else
        {
                StructName = (char *)alloca(MaxStringLength);
                sprintf(StructName, "%.*s", StringLength, ConfigFile);
        }

        size_t AllocSize = GSFileSize(ConfigFile);
        gs_buffer Buffer;
        GSBufferInit(&Buffer, (char *)alloca(AllocSize), AllocSize);

        if(!GSFileCopyToBuffer(ConfigFile, &Buffer))
                GSAbortWithMessage("Couldn't copy %s into memory\n", ConfigFile);
        Buffer.Cursor = Buffer.Start;

        GenerateSourceFile(&Buffer, ConfigCFile, StructName);

        return(EXIT_SUCCESS);
}
