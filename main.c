/******************************************************************************
 * File: main.c
 * Created: 2016-08-09
 * Last Updated: 2016-08-23
 * Creator: Aaron Oman (a.k.a GrooveStomp)
 * Notice: (C) Copyright 2016 by Aaron Oman
 *-----------------------------------------------------------------------------
 *
 * Static config generator.
 * Transforms a human-readable config file into a statically defined C struct
 * and initializer.
 *
 ******************************************************************************/
#ifndef GS_CFG_VERSION
#define GS_CFG_VERSION 0.1.0

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
        SOURCE_STYLE_C,
        SOURCE_STYLE_CASEY
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
                GSStringCopy(Name, &Self->Names[0], GSStringLength(Name));
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

                GSStringCopy(Name, &Self->Names[Self->NextNameOffset], GSStringLength(Name));
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
        if(Level == 0)
        {
                sprintf(String, "%s", "");
        }
        else
        {
                sprintf(Temp, "%%%is%c", Level * GConfig.Indent, '\0');
                sprintf(String, Temp, " ");
        }
}

unsigned int /* Resultant ConfigStack depth. */
UnwindNestedStructs(config_stack *ConfigStack, unsigned int NumSpaces, FILE *StructDefine)
{
        char IndentString[MaxStringLength];
        char Indent[MaxStringLength];
        PrintIndent(Indent, 1);

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
                fprintf(StructDefine, "%s%s} %s;\n", IndentString, Indent, StructName);
                ConfigStackDrop(ConfigStack);
        }

        return(ConfigStack->Count);
}

void
PrintFunctionIntros(FILE *Define, FILE *Init, FILE *Query, FILE *Get)
{
        /* Define */
        fprintf(Define, "#include <string.h> /* strncmp */\n");
        fprintf(Define, "typedef struct %s\n", GConfig.StructName);
        fprintf(Define, "{\n");

        /* Print Init Function Intro */
        fprintf(Init, "void\n");
        switch(GConfig.SourceStyle)
        {
                case(SOURCE_STYLE_CAMELCASE):
                {
                        fprintf(Init, "%sInit(%s *Self)\n", GConfig.StructName, GConfig.StructName);
                } break;
                case(SOURCE_STYLE_SNAKECASE):
                {
                        fprintf(Init, "%s_init(%s *self)\n", GConfig.StructName, GConfig.StructName);
                } break;
                case(SOURCE_STYLE_CASEY):
                {
                        char ModifiedStructName[MaxStringLength];
                        memset(ModifiedStructName, 0, MaxStringLength);
                        GSStringCopy(GConfig.StructName, ModifiedStructName, GSStringLength(GConfig.StructName));
                        GSStringSnakeCaseToCamelCase(ModifiedStructName, GSStringLength(ModifiedStructName));
                        fprintf(Init, "%sInit(%s *Self)\n", ModifiedStructName, GConfig.StructName);
                } break;
                default:
                {
                        fprintf(Init, "%sinit(%s *self)\n", GConfig.StructName, GConfig.StructName);
                } break;
        }
        fprintf(Init, "{\n");

        /* Print Query Function Intro */
        fprintf(Query, "unsigned int\n");
        switch(GConfig.SourceStyle)
        {
                case(SOURCE_STYLE_CAMELCASE):
                {
                        fprintf(Query, "%sHasKey(%s *Self, char *String)\n", GConfig.StructName, GConfig.StructName);
                } break;
                case(SOURCE_STYLE_SNAKECASE):
                {
                        fprintf(Query, "%s_has_key(%s *self, char *string)\n", GConfig.StructName, GConfig.StructName);
                } break;
                case(SOURCE_STYLE_CASEY):
                {
                        char ModifiedStructName[MaxStringLength];
                        memset(ModifiedStructName, 0, MaxStringLength);
                        GSStringCopy(GConfig.StructName, ModifiedStructName, GSStringLength(GConfig.StructName));
                        GSStringSnakeCaseToCamelCase(ModifiedStructName, GSStringLength(ModifiedStructName));
                        fprintf(Query, "%sHasKey(%s *Self, char *String)\n", ModifiedStructName, GConfig.StructName);
                } break;
                default:
                {
                        fprintf(Query, "%shaskey(%s *self, char *string)\n", GConfig.StructName, GConfig.StructName);
                } break;
        }
        fprintf(Query, "{\n");

        /* Print Get Function Intro */
        fprintf(Get, "char *\n");
        switch(GConfig.SourceStyle)
        {
                case(SOURCE_STYLE_CAMELCASE):
                {
                        fprintf(Get, "%sGet(%s *Self, char *String)\n", GConfig.StructName, GConfig.StructName);
                } break;
                case(SOURCE_STYLE_SNAKECASE):
                {
                        fprintf(Get, "%s_get(%s *self, char *string)\n", GConfig.StructName, GConfig.StructName);
                } break;
                case(SOURCE_STYLE_CASEY):
                {
                        char ModifiedStructName[MaxStringLength];
                        memset(ModifiedStructName, 0, MaxStringLength);
                        GSStringCopy(GConfig.StructName, ModifiedStructName, GSStringLength(GConfig.StructName));
                        GSStringSnakeCaseToCamelCase(ModifiedStructName, GSStringLength(ModifiedStructName));
                        fprintf(Get, "%sGet(%s *Self, char *String)\n", ModifiedStructName, GConfig.StructName);
                } break;
                default:
                {
                        fprintf(Get, "%sget(%s *self, char *string)\n", GConfig.StructName, GConfig.StructName);
                } break;
        }
        fprintf(Get, "{\n");
}

void
PrintFunctionSource(FILE *Init, FILE *Query, FILE *Get, char *Attribute, char *Value)
{
        char Indent[MaxStringLength];
        PrintIndent(Indent, 1);

        /* Init */
        switch(GConfig.SourceStyle)
        {
                case(SOURCE_STYLE_CASEY):
                case(SOURCE_STYLE_CAMELCASE):
                {
                        fprintf(Init, "%sSelf->%s = \"%s\";\n", Indent, Attribute, Value);
                } break;
                default:
                {
                        fprintf(Init, "%sself->%s = \"%s\";\n", Indent, Attribute, Value);
                } break;
        }

        /* Query */
        fprintf(Query, "%sif(strncmp(String, \"%s\", %lu) == 0)\n", Indent, Attribute, GSStringLength(Attribute));
        fprintf(Query, "%s{\n", Indent);
        fprintf(Query, "%s%sreturn(!0);\n", Indent, Indent);
        fprintf(Query, "%s}\n", Indent);

        /* Get */
        fprintf(Get, "%sif(strncmp(String, \"%s\", %lu) == 0)\n", Indent,  Attribute, GSStringLength(Attribute));
        fprintf(Get, "%s{\n", Indent);
        switch(GConfig.SourceStyle)
        {
                case(SOURCE_STYLE_CASEY):
                case(SOURCE_STYLE_CAMELCASE):
                {
                        fprintf(Get, "%s%sreturn(Self->%s);\n", Indent, Indent, Attribute);
                } break;
                default:
                {
                        fprintf(Get, "%s%sreturn(self->%s);\n", Indent, Indent, Attribute);
                } break;
        }
        fprintf(Get, "%s}\n", Indent);
}

void
PrintFunctionOutros(FILE *Define, FILE *Init, FILE *Query, FILE *Get)
{
        char Indent[MaxStringLength];
        PrintIndent(Indent, 1);

        /* Define */
        fprintf(Define, "} %s;\n", GConfig.StructName);
        /* Init */
        fprintf(Init, "}\n");
        /* Query */
        fprintf(Query, "%sreturn(0);\n", Indent);
        fprintf(Query, "}\n");
        /* Get */
        fprintf(Get, "%sreturn(NULL);\n", Indent);
        fprintf(Get, "}\n");
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

        char *StructInitFilename = "_struct_init.c";
        FILE *StructInit = fopen(StructInitFilename, "w");

        char *StructQueryFilename = "_struct_query.c";
        FILE *StructQuery = fopen(StructQueryFilename, "w");

        char *StructGetFilename = "_struct_get.c";
        FILE *StructGet = fopen(StructGetFilename, "w");

        PrintFunctionIntros(StructDefine, StructInit, StructQuery, StructGet);

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
                GSStringCopy(Buffer->Cursor, StringDup, Length);
                Key = strtok(StringDup, ":\n");
                Value = strtok(NULL, "\n");

                unsigned int StringLength;
                memset(Temp, 0, MaxStringLength);
                StringLength = GSMin(GSStringLength(Key), MaxStringLength);
                GSStringCopy(Key, Temp, StringLength);
                GSStringTrimWhitespace(Temp, GSStringLength(Temp));
                GSStringCopy(Temp, Key, StringLength);

                int NumSpaces;
                char Indent[MaxStringLength];
                PrintIndent(Indent, 1);

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
                        fprintf(StructDefine, "%s%sstruct\n", IndentString, Indent);
                        fprintf(StructDefine, "%s%s{\n", IndentString, Indent);

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
                fprintf(StructDefine, "%s%schar *%s;\n", IndentString, Indent, Key);
                memset(Temp, 0, MaxStringLength);
                StringLength = GSMin(GSStringLength(Value), MaxStringLength);
                GSStringCopy(Value, Temp, StringLength);
                GSStringTrimWhitespace(Temp, GSStringLength(Temp));
                GSStringCopy(Temp, Value, StringLength);

                char CompoundName[MaxStringLength];
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
                GSStringCopy(Key, CompoundNamePtr, GSStringLength(Key));
                PrintFunctionSource(StructInit, StructQuery, StructGet, CompoundName, Value);
                GSBufferNextLine(Buffer);
        }

        PrintFunctionOutros(StructDefine, StructInit, StructQuery, StructGet);

        fclose(StructDefine);
        fclose(StructInit);
        fclose(StructQuery);
        fclose(StructGet);

        int AllocSize = GSFileSize(StructDefineFilename) +
                GSFileSize(StructInitFilename) +
                GSFileSize(StructQueryFilename) +
                GSFileSize(StructGetFilename);
        gs_buffer OutputBuffer;
        GSBufferInit(&OutputBuffer, (char *)alloca(AllocSize), AllocSize);

        if(!GSFileCopyToBuffer(StructDefineFilename, &OutputBuffer))
                GSAbortWithMessage("Couldn't copy %s into memory\n", StructDefineFilename);

        if(!GSFileCopyToBuffer(StructInitFilename, &OutputBuffer))
                GSAbortWithMessage("Couldn't copy %s into memory\n", StructInitFilename);

        if(!GSFileCopyToBuffer(StructQueryFilename, &OutputBuffer))
                GSAbortWithMessage("Couldn't copy %s into memory\n", StructQueryFilename);

        if(!GSFileCopyToBuffer(StructGetFilename, &OutputBuffer))
                GSAbortWithMessage("Couldn't copy %s into memory\n", StructGetFilename);

        memset(Temp, 0, MaxStringLength);
        sprintf(Temp, "%s.c", ConfigFileBaseName);
        FILE *Out = fopen(Temp, "w");
        fwrite(OutputBuffer.Start, 1, OutputBuffer.Length, Out);
        fclose(Out);

        remove(StructDefineFilename);
        remove(StructInitFilename);
        remove(StructQueryFilename);
        remove(StructGetFilename);
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
        puts("\t         eg.: With `--struct-name config'");
        puts("\t         [CamelCase]  void configInit(config *Self);");
        puts("\t         [snake_case] void config_init(config *self);");
        puts("\t         [c]          void configinit(config *self);");
        puts("\t         [Casey]      void ConfigInit(config *Self);");
        puts("\t         If nothing is specified, defaults to `c' style.");
        puts("\t--indent: Number of spaces to indent generated source code per indentation level.");
        puts("\t          Defaults to 8.");
        exit(EXIT_SUCCESS);
}

int
main(int ArgCount, char **Arguments)
{
        gs_args *Args;
        Args = GSArgsInit(alloca(GSArgsAllocSize()), ArgCount, Arguments);
        if(GSArgsHelpWanted(Args) || ArgCount == 1) Usage(GSArgsProgramName(Args));

        char ConfigCFile[MaxStringLength];
        char *ConfigFile = GSArgsAtIndex(Args, 1);
        int StringLength = GSStringLength(ConfigFile);
        char *ExtensionStart = strchr(ConfigFile, '.');
        if(ExtensionStart != NULL)
        {
                StringLength = ExtensionStart - ConfigFile;
        }
        sprintf(ConfigCFile, "%.*s", StringLength, ConfigFile);

        char *StructName;
        if(GSArgsIsPresent(Args, "--struct-name"))
        {
                GConfig.StructName = GSArgsAfter(Args, "--struct-name");
        }
        else
        {
                GConfig.StructName = (char *)alloca(MaxStringLength);
                sprintf(GConfig.StructName, "%.*s", StringLength, ConfigFile);
        }

        GConfig.SourceStyle = SOURCE_STYLE_C;
        if(GSArgsIsPresent(Args, "--style"))
        {
                char *StyleString = GSArgsAfter(Args, "--style");
                if(GSStringIsEqual("CamelCase", StyleString, GSStringLength("CamelCase")))
                {
                        GConfig.SourceStyle = SOURCE_STYLE_CAMELCASE;
                }
                else if(GSStringIsEqual("snake_case", StyleString, GSStringLength("snake_case")))
                {
                        GConfig.SourceStyle = SOURCE_STYLE_SNAKECASE;
                }
                else if(GSStringIsEqual("Casey", StyleString, GSStringLength("Casey")))
                {
                        GConfig.SourceStyle = SOURCE_STYLE_CASEY;
                }
        }

        if(GSArgsIsPresent(Args, "--indent"))
        {
                char *Indent = GSArgsAfter(Args, "--indent");
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

#endif /* GS_CFG_VERSION */
