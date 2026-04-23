#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef void (*pvHandlerFuction)(uint8_t* pu8Argv);

typedef struct {
    const char *pcCommandLine;
    pvHandlerFuction pvHandlerFunction;
} CommandLine_t ;

void mallocCmd(uint8_t *argv) {
    printf("malloc cmd with input: %s\n", argv);
}

void freeCmd(uint8_t *argv) {
    printf("free_cmd with input: %s\n", argv);
}

CommandLine_t commandTable[] = {
    {"malloc", mallocCmd},
    {"free", freeCmd},
    {NULL, NULL}
};

bool bCommandParser(CommandLine_t *pxCommandLineTable, char *pcCommandLine)
{
    char *pcCurrentCharacter = pcCommandLine;
    char *pcCommandLineHandler = malloc(32);

    if (pxCommandLineTable == NULL || pcCommandLine == NULL || pcCommandLineHandler == NULL) {
        return false;
    }

    char *pcStart = pcCommandLineHandler;

    while (*pcCurrentCharacter) {
        if ((*pcCurrentCharacter == ' ') || (*pcCurrentCharacter == '\r') || (*pcCurrentCharacter == '\n')) {
            break;
        } else {
            *pcCommandLineHandler++ = *pcCurrentCharacter++;
        }
    }

    *pcCommandLineHandler = '\0';

    while (pxCommandLineTable->pcCommandLine != NULL) {

        if (strcmp(pxCommandLineTable->pcCommandLine, pcStart) == 0) {
            pxCommandLineTable->pvHandlerFunction(pcStart);
            free(pcStart);
            return true;
        }

        pxCommandLineTable++;
    }

    free(pcStart);

    return true;
}

int main()
{
    char input[] = "free 100\n";

    bCommandParser(commandTable, input);

    return 0;
}