/******************************************************************************
 * File: main.c
 * Created: 2016-08-09
 * Last Updated: 2016-08-10
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

#define NULL_CHAR '\0'
const int MaxStringLength = 255;
const int MaxNestedStructs = 10;

typedef enum source_style_e
{
        SOURCE_STYLE_CAMELCASE,
        SOURCE_STYLE_SNAKECASE,
        SOURCE_STYLE_C
} source_style_e;

typedef struct config
{
        char *StructName;
        source_style_e SourceStyle;
        int Indent;
} config;

typedef struct config_stack
{
        char *Names;
        int *NameOffsets;
        unsigned int NextNameOffset;
        int *Indentation;
        unsigned int Count;
} config_stack;

/******************************************************************************
 * Globals
 ******************************************************************************/

config GConfig;

/******************************************************************************
 * Functions
 ******************************************************************************/

void
ConfigStackInit(config_stack *Self)
{
        Self->Names = (char *)malloc(MaxStringLength * MaxNestedStructs);
        memset(Self->Names, 0, MaxStringLength * MaxNestedStructs);

        Self->NameOffsets = (int *)malloc(sizeof(int) * MaxNestedStructs);
        memset(Self->NameOffsets, -1, MaxNestedStructs);

        Self->Indentation = (int *)malloc(sizeof(int) * MaxNestedStructs);
        memset(Self->Indentation, -1, MaxNestedStructs);

        Self->NextNameOffset = 0;
        Self->Count = 0;
}

void
ConfigStackDestroy(config_stack *Self)
{
        free(Self->Names);
        free(Self->NameOffsets);
        free(Self->Indentation);
}

unsigned int
ConfigStackDepth(config_stack *Self)
{
        return(Self->Count);
}

char * /* Returns NULL if Index is out of bounds. */
ConfigStackNameAt(config_stack *Self, unsigned int Index)
{
        if(Index >= Self->Count) return(NULL);

        int NameIndex = Self->NameOffsets[Index];
        char *Result = &Self->Names[NameIndex];

        return(Result);
}

int /* Returns -1 if Index is out of range. */
ConfigStackIndentationAt(config_stack *Self, unsigned int Index)
{
        if(Index >= Self->Count) return(-1);

        int Result = (int)Self->Indentation[Index];

        return(Result);
}

gs_bool
ConfigStackDrop(config_stack *Self)
{
        if(Self->Count <= 0) return(true);

        int NameIndex = Self->NameOffsets[Self->Count - 1];
        Self->Names[NameIndex] = NULL_CHAR;
        Self->NextNameOffset = NameIndex;
        Self->Indentation[Self->Count - 1] = -1;
        Self->Count--;
        return(true);
}

gs_bool
ConfigStackAdd(config_stack *Self, char *Name, unsigned int Indentation)
{
        if(Self->Count == 0)
        {
                GSStringCopyWithNull(Name, &Self->Names[0], GSStringLength(Name));
                Self->NameOffsets[0] = 0;
                Self->Count = 1;
                Self->NextNameOffset = GSStringLength(Name) + 1;
                Self->Indentation[0] = Indentation;
        }
        else
        {
                int StringLength = GSStringLength(Name);

                if((StringLength + Self->NextNameOffset) > (MaxStringLength * MaxNestedStructs))
                {
                        return(false);
                }

                GSStringCopyWithNull(Name, &Self->Names[Self->NextNameOffset], GSStringLength(Name));
                Self->NameOffsets[Self->Count] = Self->NextNameOffset;
                Self->Indentation[Self->Count] = Indentation;
                Self->Count++;
                Self->NextNameOffset += (StringLength + 1);
        }

        return(true);
}

void
PrintIndent(char *String, unsigned int Level)
{
        char Temp[255];
        sprintf(Temp, "%%%is%c", Level * 8, '\0');
        sprintf(String, Temp, " ");
}

unsigned int /* Resultant ConfigStack depth. */
UnwindNestedStructs(config_stack *ConfigStack, unsigned int NumSpaces, FILE *StructDefine)
{
        char IndentString[MaxStringLength];
        int StructIndex = -1;
        for(int I = 0; I < ConfigStack->Count; I++)
        {
                int Indentation = ConfigStackIndentationAt(ConfigStack, I);
                if(-1 == Indentation) GSAbortWithMessage("This is unexpected!\n");

                if(NumSpaces == Indentation)
                {
                        StructIndex = I;
                        break;
                }
        }

        if(-1 == StructIndex && NumSpaces != 0) GSAbortWithMessage("This is unexpected!\n");

        for(int I = ConfigStack->Count - 1; I > StructIndex; I--)
        {
                PrintIndent(IndentString, ConfigStack->Count - 1);
                char *StructName = ConfigStackNameAt(ConfigStack, I);
                fprintf(StructDefine, "%s        } %s;\n", IndentString, StructName);
                ConfigStackDrop(ConfigStack);
        }

        return(ConfigStack->Count);
}

void
PrintInitFunctionOpening(FILE *File)
{
        fprintf(File, "void\n");
        switch(GConfig.SourceStyle)
        {
                case(SOURCE_STYLE_CAMELCASE):
                {
                        fprintf(File, "%sInit(%s *Self)\n", GConfig.StructName, GConfig.StructName);
                } break;
                case(SOURCE_STYLE_SNAKECASE):
                {
                        fprintf(File, "%s_init(%s *self)\n", GConfig.StructName, GConfig.StructName);
                } break;
                default:
                {
                        fprintf(File, "%sinit(%s *self)\n", GConfig.StructName, GConfig.StructName);
                } break;
        }
        fprintf(File, "{\n");
}

void
PrintInitFunctionSourceLine(FILE *File, char *Attribute, char *Value)
{
        switch(GConfig.SourceStyle)
        {
                case(SOURCE_STYLE_CAMELCASE):
                {
                        fprintf(File, "        Self->%s = \"%s\";\n", Attribute, Value);
                } break;
                default:
                {
                        fprintf(File, "        self->%s = \"%s\";\n", Attribute, Value);
                } break;
        }
}

void
GenerateSourceFile(gs_buffer *Buffer, char *ConfigFileBaseName)
{
        char *Key;
        char *Value;
        char Temp[MaxStringLength];
        config_stack ConfigStack;
        ConfigStackInit(&ConfigStack);
        char IndentString[MaxStringLength];
        PrintIndent(IndentString, ConfigStack.Count);

        char *StructDefineFilename = "_struct_define.c";
        FILE *StructDefine = fopen(StructDefineFilename, "w");
        fprintf(StructDefine, "typedef struct %s\n", GConfig.StructName);
        fprintf(StructDefine, "{\n");

        char *StructInitFilename = "_struct_init.c";
        FILE *StructInit = fopen(StructInitFilename, "w");

        PrintInitFunctionOpening(StructInit);

        while(true)
        {
                if(GSBufferIsEOF(Buffer))
                {
                        if(ConfigStack.Count > 0)
                        {
                                UnwindNestedStructs(&ConfigStack, 0, StructDefine);
                        }
                        break;
                }

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

                int Length = Newline - Buffer->Cursor;
                char *StringDup = (char *)alloca(Length + 1);
                GSStringCopyWithNull(Buffer->Cursor, StringDup, Length);
                Key = strtok(StringDup, ":\n");
                Value = strtok(NULL, "\n");

                unsigned int StringLength;
                memset(Temp, 0, MaxStringLength);
                StringLength = GSMin(GSStringLength(Key), MaxStringLength);
                StringLength = GSStringCopyWithoutSurroundingWhitespaceWithNull(Key, Temp, StringLength);
                GSStringCopyWithNull(Temp, Key, StringLength);

                int NumSpaces;

                if(Value == NULL)
                {
                        /* Get the indentation level for this nested struct. */
                        GSBufferSaveCursor(Buffer);
                        GSBufferNextLine(Buffer);
                        for(NumSpaces = 0; GSCharIsWhitespace(Buffer->Cursor[NumSpaces]); NumSpaces++);
                        GSBufferRestoreCursor(Buffer);

                        /* Nested struct definition */
                        PrintIndent(IndentString, ConfigStack.Count);
                        ConfigStackAdd(&ConfigStack, Key, NumSpaces);
                        fprintf(StructDefine, "%s        struct\n", IndentString);
                        fprintf(StructDefine, "%s        {\n", IndentString);

                        /* Advanced and continue with the parsing. */
                        GSBufferNextLine(Buffer);
                        continue;
                }

                for(NumSpaces = 0; GSCharIsWhitespace(Buffer->Cursor[NumSpaces]); NumSpaces++);

                if(ConfigStack.Count > 0)
                {
                        UnwindNestedStructs(&ConfigStack, NumSpaces, StructDefine);
                }

                PrintIndent(IndentString, ConfigStack.Count);
                fprintf(StructDefine, "%s        char *%s;\n", IndentString, Key);
                memset(Temp, 0, MaxStringLength);
                StringLength = GSMin(GSStringLength(Value), MaxStringLength);
                StringLength = GSStringCopyWithoutSurroundingWhitespace(Value, Temp, StringLength);
                GSStringCopyWithNull(Temp, Value, StringLength);

                char CompoundName[256];
                char *CompoundNamePtr = CompoundName;
                if(ConfigStack.Count > 0)
                {
                        for(int i=0; i<ConfigStack.Count; i++)
                        {
                                char *CurrentName = ConfigStackNameAt(&ConfigStack, i);
                                int StringLength = GSStringLength(CurrentName);
                                sprintf(CompoundNamePtr, "%s.", CurrentName);
                                CompoundNamePtr = CompoundNamePtr + StringLength + 1;
                        }
                }
                GSStringCopyWithNull(Key, CompoundNamePtr, GSStringLength(Key));
                PrintInitFunctionSourceLine(StructInit, CompoundName, Value);
                GSBufferNextLine(Buffer);
        }

        fprintf(StructDefine, "} %s;\n", GConfig.StructName);
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
        ConfigStackDestroy(&ConfigStack);
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
        puts("\t--style: One of: CamelCase, snake_case, c");
        puts("\t         This affects the initialization function generated for the config struct.");
        puts("\t         eg.: With `--struct-name config_t'");
        puts("\t         [CamelCase]  void config_tInit(config_t *Self);");
        puts("\t         [snake_case] void config_t_init(config_t *Self);");
        puts("\t         [c]          void config_tinit(config_t *Self);");
        puts("\t         If nothing is specified, defaults to `c' style.");
        puts("\t--indent: Number of spaces to indent generated source code per indentation level.");
        puts("\t          Defaults to 8.");
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
                GConfig.StructName = GSArgAfter(&Args, "--struct-name");
        }
        else
        {
                GConfig.StructName = (char *)alloca(MaxStringLength);
                sprintf(StructName, "%.*s", StringLength, ConfigFile);
        }

        GConfig.SourceStyle = SOURCE_STYLE_C;
        if(GSArgIsPresent(&Args, "--style"))
        {
                char *StyleString = GSArgAfter(&Args, "--style");
                if(GSStringIsEqual("CamelCase", StyleString, GSStringLength("CamelCase")))
                {
                        GConfig.SourceStyle = SOURCE_STYLE_CAMELCASE;
                }
                else if(GSStringIsEqual("snake_case", StyleString, GSStringLength("snake_case")))
                {
                        GConfig.SourceStyle = SOURCE_STYLE_SNAKECASE;
                }
        }

        if(GSArgIsPresent(&Args, "--indent"))
        {
                char *Indent = GSArgAfter(&Args, "--indent");
                GConfig.Indent = strtol(Indent, NULL, 10);
        }
        else
        {
                GConfig.Indent = 8;
        }

        size_t AllocSize = GSFileSize(ConfigFile);
        gs_buffer Buffer;
        GSBufferInit(&Buffer, (char *)alloca(AllocSize), AllocSize);

        if(!GSFileCopyToBuffer(ConfigFile, &Buffer))
                GSAbortWithMessage("Couldn't copy %s into memory\n", ConfigFile);
        Buffer.Cursor = Buffer.Start;

        GenerateSourceFile(&Buffer, ConfigCFile);

        return(EXIT_SUCCESS);
}
