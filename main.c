#include <alloca.h>
#include <stdio.h>
#include <stdlib.h> /* EXIT_SUCCESS */
#include <libgen.h> /* POSIX basename */
#include "gs.h"

typedef struct buffer
{
        char *Start;
        char *Cursor;
        size_t Capacity;
        size_t Length;
} buffer;

buffer *
BufferSet(buffer *Buffer, char *Start, size_t Length, size_t Capacity)
{
        Buffer->Start = Start;
        Buffer->Cursor = Start;
        Buffer->Length = Length;
        Buffer->Capacity = Capacity;
        return(Buffer);
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
        return(FileSize);
}

gs_bool
CopyFileIntoBuffer(char *FileName, buffer *Buffer)
{
        FILE *File = fopen(FileName, "r");
        if(File != NULL)
        {
                fseek(File, 0, SEEK_END);
                size_t FileSize = ftell(File);
                if(FileSize > (Buffer->Capacity - (Buffer->Cursor - Buffer->Start)))
                {
                        return(false);
                }
                fseek(File, 0, SEEK_SET);
                fread(Buffer->Cursor, 1, FileSize, File);
                Buffer->Length = FileSize;
        }

        return(true);
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

        char *ConfigFile = GSArgAtIndex(ArgCount, Args, 1);
        int StringLength = GSStringLength(ConfigFile);
        char *ExtensionStart = strchr(ConfigFile, '.');
        if(ExtensionStart != NULL)
        {
                StringLength = ExtensionStart - ConfigFile;
        }

        printf("Generating %.*s.c for %s\n", StringLength, ConfigFile, ConfigFile);

        size_t AllocSize = FileSize(ConfigFile);
        buffer Buffer;
        BufferSet(&Buffer, (char *)alloca(AllocSize), 0, AllocSize);
        gs_bool IsCopied = CopyFileIntoBuffer(ConfigFile, &Buffer);
        if(!IsCopied) GSAbortWithMessage("Couldn't copy ConfigFile into memory\n");

        printf("File in memory:\n%.*s\n", (int)Buffer.Length, Buffer.Start);

        return(EXIT_SUCCESS);
}
