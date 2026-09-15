/**
 * Project 1
 * Assembler code fragment for LC-2K
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ctype.h>

// Every LC2K file will contain less than 1000 lines of assembly.
#define MAXLINELENGTH 1000
#define MAXLABELS 1000
#define MAXLABELCHARS 7

int readAndParse(FILE *, char *, char *, char *, char *, char *);
static void checkForBlankLinesInCode(FILE *inFilePtr);
static inline int isNumber(char *);
static inline void printHexToFile(FILE *, int);
static int endsWith(char *, char *);

static int parseRegister(char *input);
static int lookupLabel(char *desiredLabelName, char labelTable[MAXLABELS][MAXLABELCHARS], int labelAddress[MAXLABELS], int labelCount);
static int resolveOffset(char *offsetField, int currInstructionAddress, bool isPCRelativeBranch, char labelTable[MAXLABELS][MAXLABELCHARS], int labelAddress[MAXLABELS], int labelCount);
static void validateLabel(char *label);

int main(int argc, char **argv)
{
    char *inFileString, *outFileString;
    FILE *inFilePtr, *outFilePtr;
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
        arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];

    char labelTable[MAXLABELS][MAXLABELCHARS]; // labelTable[index] = one label string
    int labelAddress[MAXLABELS];               // labelAddress[index] = that label’s address
    int labelCount = 0;
    int address;

    if (argc != 3)
    {
        printf("error: usage: %s <assembly-code-file> <machine-code-file>\n",
               argv[0]);
        exit(1);
    }

    inFileString = argv[1];
    outFileString = argv[2];

    if (!endsWith(inFileString, ".as") &&
        !endsWith(inFileString, ".s") &&
        !endsWith(inFileString, ".lc2k"))
    {
        printf("warning: assembly code file does not end with .as, .s, or .lc2k\n");
    }

    if (!endsWith(outFileString, ".mc"))
    {
        printf("error: machine code file must end with .mc\n");
        exit(1);
    }

    inFilePtr = fopen(inFileString, "r");
    if (inFilePtr == NULL)
    {
        printf("error in opening %s\n", inFileString);
        exit(1);
    }

    // Check for blank lines in the middle of the code.
    checkForBlankLinesInCode(inFilePtr);

    outFilePtr = fopen(outFileString, "w");
    if (outFilePtr == NULL)
    {
        printf("error in opening %s\n", outFileString);
        exit(1);
    }

    // PASS 1!!!!!
    address = 0;

    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2))
    {
        if (label[0] != '\0')
        {
            validateLabel(label);
            if (labelCount >= MAXLABELS)
                exit(1);

            for (int i = 0; i < labelCount; i++)
            {
                if (strcmp(labelTable[i], label) == 0)
                    exit(1); // duplicate def
            }
            strcpy(labelTable[labelCount], label);
            labelAddress[labelCount] = address;
            labelCount++;
        }
        address++;
    }

    rewind(inFilePtr);

    // PASS 2!!!!!!!!!!!

    address = 0;

    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2))
    {
        int machineCode = 0;
        if (strcmp(opcode, ".fill") == 0)
        {
            if (isNumber(arg0))
            {
                machineCode = atoi(arg0);
            }
            else
            {
                machineCode = lookupLabel(arg0, labelTable, labelAddress, labelCount);
            }
        }
        else if (strcmp(opcode, "add") == 0 || strcmp(opcode, "nor") == 0)
        {
            int regA = parseRegister(arg0);
            int regB = parseRegister(arg1);
            int regDest = parseRegister(arg2);
            int opcodeVal = (strcmp(opcode, "add") == 0) ? 0 : 1;
            machineCode = (opcodeVal << 22) | (regA << 19) | (regB << 16) | regDest;
        }
        else if (strcmp(opcode, "lw") == 0 || strcmp(opcode, "sw") == 0 || strcmp(opcode, "beq") == 0)
        {
            int regA = parseRegister(arg0);
            int regB = parseRegister(arg1);
            int offset = resolveOffset(arg2, address, strcmp(opcode, "beq") == 0, labelTable, labelAddress, labelCount);

            int opcodeVal = -1;
            if (strcmp(opcode, "lw") == 0)
            {
                opcodeVal = 2;
            }
            else if (strcmp(opcode, "sw") == 0)
            {
                opcodeVal = 3;
            }
            else if (strcmp(opcode, "beq") == 0)
            {
                opcodeVal = 4;
            }

            machineCode = (opcodeVal << 22) | (regA << 19) | (regB << 16) | (offset & 0xFFFF);
        }
        else if (strcmp(opcode, "jalr") == 0)
        {
            int regA = parseRegister(arg0);
            int regB = parseRegister(arg1);
            int opcodeVal = 5;
            machineCode = (opcodeVal << 22) | (regA << 19) | (regB << 16);
        }
        else if (strcmp(opcode, "halt") == 0)
        {
            machineCode = 6 << 22;
        }
        else if (strcmp(opcode, "noop") == 0)
        {
            machineCode = 7 << 22;
        }
        else if (strcmp(opcode, "b") == 0)
        {
            int offset = resolveOffset(arg0, address, true, labelTable, labelAddress, labelCount);
            machineCode = (4 << 22) | (offset & 0xFFFF);
        }
        else if (strcmp(opcode, "jump") == 0)
        {
            int regA = parseRegister(arg0);
            int opcode = 5;
            machineCode = (opcode << 22) | (regA << 19);
        }
        else
        {
            exit(1);
        }

        printHexToFile(outFilePtr, machineCode);
        address++;
    }

    fclose(inFilePtr);
    fclose(outFilePtr);

    return 0;
}

// Returns non-zero if the line contains only whitespace.
static int lineIsBlank(char *line)
{
    char whitespace[4] = {'\t', '\n', '\r', ' '};
    int nonempty_line = 0;
    for (int line_idx = 0; line_idx < strlen(line); ++line_idx)
    {
        int line_char_is_whitespace = 0;
        for (int whitespace_idx = 0; whitespace_idx < 4; ++whitespace_idx)
        {
            if (line[line_idx] == whitespace[whitespace_idx])
            {
                line_char_is_whitespace = 1;
                break;
            }
        }
        if (!line_char_is_whitespace)
        {
            nonempty_line = 1;
            break;
        }
    }
    return !nonempty_line;
}

// Exits 2 if file contains an empty line anywhere other than at the end of the file.
// Note calling this function rewinds inFilePtr.
static void checkForBlankLinesInCode(FILE *inFilePtr)
{
    char line[MAXLINELENGTH];
    int blank_line_encountered = 0;
    int address_of_blank_line = 0;
    rewind(inFilePtr);

    for (int address = 0; fgets(line, MAXLINELENGTH, inFilePtr) != NULL; ++address)
    {
        // Check for line too long
        if (strlen(line) >= MAXLINELENGTH - 1)
        {
            printf("error: line too long\n");
            exit(1);
        }

        // Check for blank line.
        if (lineIsBlank(line))
        {
            if (!blank_line_encountered)
            {
                blank_line_encountered = 1;
                address_of_blank_line = address;
            }
        }
        else
        {
            if (blank_line_encountered)
            {
                printf("Invalid Assembly: Empty line at address %d\n", address_of_blank_line);
                exit(2);
            }
        }
    }
    rewind(inFilePtr);
}

/*
 * NOTE: The code defined below is not to be modifed as it is implimented correctly.
 */

/*
 * Read and parse a line of the assembly-language file.  Fields are returned
 * in label, opcode, arg0, arg1, arg2 (these strings must have memory already
 * allocated to them).
 *
 * Return values:
 *     0 if reached end of file
 *     1 if all went well
 *
 * exit(1) if line is too long.
 */
int readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0,
                 char *arg1, char *arg2)
{
    char line[MAXLINELENGTH];
    char *ptr = line;

    /* delete prior values */
    label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';

    /* read the line from the assembly-language file */
    if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL)
    {
        /* reached end of file */
        return (0);
    }

    /* check for line too long */
    if (strlen(line) == MAXLINELENGTH - 1)
    {
        printf("error: line too long\n");
        exit(1);
    }

    // Ignore blank lines at the end of the file.
    if (lineIsBlank(line))
    {
        return 0;
    }

    /* is there a label? */
    ptr = line;
    if (sscanf(ptr, "%[^\t\n ]", label))
    {
        /* successfully read label; advance pointer over the label */
        ptr += strlen(label);
    }

    /*
     * Parse the rest of the line.  Would be nice to have real regular
     * expressions, but scanf will suffice.
     */
    sscanf(ptr, "%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]",
           opcode, arg0, arg1, arg2);

    return (1);
}

static inline int
isNumber(char *string)
{
    int num;
    char c;
    return ((sscanf(string, "%d%c", &num, &c)) == 1);
}

// Prints a machine code word in the proper hex format to the file
static inline void
printHexToFile(FILE *outFilePtr, int word)
{
    fprintf(outFilePtr, "0x%08X\n", word);
}

// Returns 1 if string ends with substr, 0 otherwise
static int
endsWith(char *string, char *substr)
{
    size_t stringLen = strlen(string);
    size_t substrLen = strlen(substr);
    if (stringLen < substrLen)
    {
        return 0; // string too short
    }
    char *stringEnd = string + stringLen - substrLen;
    if (strcmp(stringEnd, substr) == 0)
    {
        return 1;
    }
    return 0;
}

// MY HELPER FUNCTIONS!!
static int parseRegister(char *input)
{
    if (!isNumber(input))
        exit(1);
    int regNum = atoi(input);
    if (regNum < 0 || regNum > 7)
        exit(1);
    return regNum;
}

static int lookupLabel(char *desiredLabelName, char labelTable[MAXLABELS][MAXLABELCHARS], int labelAddress[MAXLABELS], int labelCount)
{
    for (int i = 0; i < labelCount; i++)
        if (strcmp(desiredLabelName, labelTable[i]) == 0)
            return labelAddress[i];
    exit(1);
}

static int resolveOffset(char *offsetField, int currInstructionAddress, bool isPCRelativeBranch, char labelTable[MAXLABELS][MAXLABELCHARS], int labelAddress[MAXLABELS], int labelCount)
{
    int offset = 0;
    if (isNumber(offsetField))
    {
        offset = atoi(offsetField);
    }
    else
    {
        int targetAddress = lookupLabel(offsetField, labelTable, labelAddress, labelCount);

        if (isPCRelativeBranch)
        {
            offset = targetAddress - (currInstructionAddress + 1);
        }
        else
        {
            offset = targetAddress;
        }
    }

    if (offset < -32768 || offset > 32767)
        exit(1);

    return offset;
}

static void validateLabel(char *label)
{
    int len = strlen(label);

    if (len == 0 || len > 6)
        exit(1);
    if (!isalpha((unsigned char)label[0]))
        exit(1);
    for (int i = 0; i < len; i++)
    {
        if (!isalnum((unsigned char)label[i]))
            exit(1);
    }
    return;
}
