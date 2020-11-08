/*------------------------------------------------------------------------------------------------------
--
-- pasm.c
-- An assembler for the pumpkin-cpu
-- Version 1.3
--
--------------------------------------------------------------------------------------------------------
--
-- This file is part of the pumpkin-cpu Project
-- Copyright (C) 2020 Steve Teal
-- 
-- This source file may be used and distributed without restriction provided that this copyright
-- statement is not removed from the file and that any derivative work contains the original
-- copyright notice and the associated disclaimer.
-- 
-- This source file is free software; you can redistribute it and/or modify it under the terms
-- of the GNU Lesser General Public License as published by the Free Software Foundation,
-- either version 3 of the License, or (at your option) any later version.
-- 
-- This source is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
-- without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
-- See the GNU Lesser General Public License for more details.
-- 
-- You should have received a copy of the GNU Lesser General Public License along with this
-- source; if not, download it from http://www.gnu.org/licenses/lgpl-3.0.en.html
--
------------------------------------------------------------------------------------------------------*/
#include<stdio.h>
#include<conio.h>
#include<string.h>
#include<stdlib.h>
#include<ctype.h>
#include<time.h>

#define VERSION_STRING         "1.3" 
#define MAX_MEMORY_SIZE        (4096)
#define MAX_LABEL_NAME_LENGTH  (64)
#define MAX_LABELS             (500)
#define MAX_IMMEDIATES         (500)
#define MAX_LINE_LENGTH        (256)
#define MAX_WORDS_ON_LINE      (5) /* eg: LABEL DB 0 DUP 125 */
#define NUMBER_OF_INSTRUCTIONS (16)
#define DB_DW_BUFFER_SIZE      (256)

char *instructions[] = {"LOAD","STORE","ADD","SUB","OR","AND","XOR","ROR","SWAP","IN","OUT","BR","BNC","BNZ","CALL","RETURN"};

char VHDLFileStart[] =
    "---------------------------------------------------------------------\n"
    "--\n"
    "-- Built with PASM version %s\n"
    "-- File name: %s\n"
    "-- %s\n"
    "-- \n"
    "---------------------------------------------------------------------\n"
    "library ieee;\n"
    "use ieee.std_logic_1164.all;\n"
    "use ieee.numeric_std.all;\n\n"
    
    "entity %s is\n"
    "port (\n"
    "    clock        : in std_logic;\n"
    "    clock_enable : in std_logic;\n"
    "    address      : in std_logic_vector(%d downto 0);\n"
    "    data_out     : out std_logic_vector(15 downto 0);\n"
    "    data_in      : in std_logic_vector(15 downto 0);\n"
    "    write_enable : in std_logic);\n"
    "end entity;\n\n"
    "architecture rtl of %s is\n\n"
    "    type ram_type is array (0 to %d) of std_logic_vector(15 downto 0);\n"
    "    signal ram : ram_type := (\n";

char VHDLFileEnd[] =
    "begin\n\n"
    "    process(clock)\n"
    "    begin\n"
    "        if rising_edge(clock) then\n"
    "            if clock_enable = '1' then\n"
    "                if write_enable = '1' then\n"
    "                    ram(to_integer(unsigned(address))) <= data_in;\n"
    "                else\n"
    "                    data_out <= ram(to_integer(unsigned(address)));\n"
    "                end if;\n"
    "            end if;\n"
    "        end if;\n"
    "    end process;\n\n"
    "end rtl;\n\n"
    "--- End of file ---\n";


typedef struct 
{
    char name[MAX_LABEL_NAME_LENGTH+1]; /* +1 to allow space for null */
    int value;
}label_t;

typedef struct
{
    int value;
    int address;
}immediate_t;

int memorySize; /* Size of target memory */
int memoryImage[MAX_MEMORY_SIZE]; /* Assembled memory image */
int currentAddress;
int endAddress;
int currentLine; 
int numImmediates;
immediate_t immediates[MAX_IMMEDIATES];
int numLabels;
label_t labels[MAX_LABELS];
int errorCount;
char line[MAX_LINE_LENGTH+1];
char *words[MAX_WORDS_ON_LINE];
int wordCount;
int buffer[DB_DW_BUFFER_SIZE]; /* Output storage for DB and DW parsers */
int bufferIndex;
int dup; /* DUP value used by DB/DW parsers */
int pass; /* Which pass we're on */

/*
    Remove path from fileName
*/

void removePath(char *fileName, char *filePath, int size)
{
    int i;

    for(i=strlen(filePath);i>0;i--)
    {
        if(filePath[i-1]=='/' || filePath[i-1]=='\\')
        {
            break;
        }
    }

    strncpy(fileName,&filePath[i],size);
}

/*
    Remove extension from fileName
*/

void removeExtension(char *noExt, char *fileName, int size)
{
    int i;

    for(i=strlen(fileName);i>0;i--)
    {
        if(fileName[i]=='.')
        {
            break;
        }
    }

    if(i == 0)
    {
        i = strlen(fileName);
    }

    if(i > size)
    {
        i = size;
    }

    strncpy(noExt,fileName,i);
    noExt[i] = 0;
}

/*
    Return pointer to filename extension
*/

char *getExtension(char *fileName)
{
    char *ptr;

    ptr = fileName;

    if(strlen(fileName)>3)
    {
        ptr += strlen(fileName);
        do
        {
            ptr--;
            if(*ptr == '.')
            {
                return ptr+1;
            }
        }
        while(ptr != fileName);
    }
    
    return NULL;
}

/*
    Get current date and time and return as string
*/

void getTimeDate(char *str, int size)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(str,size,"%d-%d-%d %02d:%02d:%02d",tm.tm_mday,tm.tm_mon + 1,tm.tm_year + 1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
}

int bit_width(int m)
{
    int i;

    i = 0;
    while(m > 2)
    {
        i++;
        m >>= 1;
    }

    return i;
}


/*
    Create a VHDL file of a RAM model initialized with the assembled memory image 
*/
void createVHDLFile(char *fileName)
{
    FILE *fp;
    int i;
    char entity[40];
    char file[40];
    char dateTime[80];

    fp = fopen(fileName,"w");
    if(fp == NULL)
    {
        fclose(fp);
        printf("Could not open output file %s\n",fileName);
    }
    else
    {       
        removePath(file, fileName, sizeof(file));
        removeExtension(entity, file, sizeof(entity));
        getTimeDate(dateTime, sizeof(dateTime));
        fprintf(fp, VHDLFileStart, VERSION_STRING, file, dateTime, entity, bit_width(memorySize), entity, memorySize-1);
        fputs("\t\t\t",fp);
        for(i=0;i<memorySize;i++)
        {
            fprintf(fp, "X\"%04X\"",memoryImage[i]);
            if(i<memorySize-1)
            {
                fputc(',',fp);
                if((i+1)%8 == 0)
                {
                    fputs("\n\t\t\t",fp);
                }
            }
            else
            {
                fputs(");\n",fp);
            }            
        }
        fprintf(fp,VHDLFileEnd);
        fclose(fp);
        printf("VHDL file '%s' created.\n",fileName);
    }
}

/*
    Create MIF file
*/

void createMIFFile(char *fileName)
{
    FILE *fp;
    int i;
    char file[40];
    char dateTime[80];

    fp = fopen(fileName,"w");
    if(fp == NULL)
    {
        fclose(fp);
        printf("Could not open output file %s\n",fileName);
    }
    else
    {       
        removePath(file, fileName, sizeof(file));
        getTimeDate(dateTime, sizeof(dateTime));
        fprintf(fp, "-- Built with PASM version %s\n",VERSION_STRING);
        fprintf(fp, "-- File name: %s\n",file);
        fprintf(fp, "-- %s\n\n",dateTime );
        fprintf(fp, "DEPTH = %d;\n", memorySize);
        fprintf(fp, "WIDTH = 16;\n");
        fprintf(fp, "ADDRESS_RADIX = HEX;\n");
        fprintf(fp, "DATA_RADIX = HEX;\n");
        fprintf(fp, "CONTENT\nBEGIN\n");
        for(i=0;i<memorySize;i++)
        {
            fprintf(fp, "%03X : %04X ;\n",i,memoryImage[i]);
        }
        fprintf(fp, "END;\n");
        fclose(fp);
        printf("MIF file '%s' created.\n",fileName);
    }
}

/*
    Create MEM file
*/

void createMEMFile(char *fileName)
{
    FILE *fp;
    int i;
    char file[40];
    char dateTime[80];


    fp = fopen(fileName,"w");
    if(fp == NULL)
    {
        fclose(fp);
        printf("Could not open output file %s\n",fileName);
    }
    else
    {       
        removePath(file, fileName, sizeof(file));
        getTimeDate(dateTime, sizeof(dateTime));
        fprintf(fp, "#Format=AddrHex\n");
        fprintf(fp, "#Depth=%d\n",memorySize);
        fprintf(fp, "#Width=16\n");
        fprintf(fp, "#AddrRadix=3\n");
        fprintf(fp, "#DataRadix=3\n");
        fprintf(fp, "#Data\n");
        fprintf(fp, "#Built with PASM version %s\n",VERSION_STRING);
        fprintf(fp, "#File name: %s\n",file);
        fprintf(fp, "#%s\n",dateTime );
        for(i=0;i<memorySize;i++)
        {
            fprintf(fp, "%03X : %04X\n",i,memoryImage[i]);
        }
        fprintf(fp, "# The end\n");
        fclose(fp);
        printf("MEM file '%s' created.\n",fileName);
    }
}

/* 
    Split line into words delimited by whitespace 
*/
void splitLine(void)
{
    int i;
    int nextWord;
    int lineLength;
    int quote; 

    wordCount = 0;
    nextWord = 1; /* Searching for start of next word */
    quote = 0; /* Ignore spaces inside quotes */
    lineLength = strlen(line);
    for(i=0;i<lineLength;i++)
    {
        /* End of line or start of comment */
        if(line[i] == 0 || line[i] == ';')
        {
            break;
        }
        /* Toggle quotes (used to ignore spaces inside quotes) */
        if(line[i] == '\"')
        {
            quote ^= 1;            
        }        
        if(nextWord == 1 && !isspace(line[i]))
        {
            /* Start of next word */
            words[wordCount++] = &line[i];
            nextWord = 0;
            if(wordCount > MAX_WORDS_ON_LINE)
            {
                printf("Error line %d: too many words\n",currentLine);
                errorCount++;
                wordCount = 0;
                return;
            }            
            continue;
        }
        if(nextWord == 0 && isspace(line[i]) && quote == 0)
        {
            /* End of current word - mark with null */
            line[i] = 0; 
            nextWord = 1;
        }
    }

    /* Check if quotes are still open */
    if(quote)
    {
        printf("Error line %d: no closing \"\n",currentLine);
        errorCount++;
        wordCount = 0;
    }
}

/*
    Return instruction opcode (0 to 15) or -1 if word is not an instruction 
*/

int getInstruction(char *word)
{
    int i;

    for(i=0;i<NUMBER_OF_INSTRUCTIONS;i++)
    {
        if(strcmp(instructions[i],word)==0)
        {
            return i;
        }
    }

    return -1;
}

/* 
    Perform various checks on newLabel if all is OK add to label list
*/

int addLabel(char *newLabel)
{
    int i;

    /* Check label is not too long */
    if(strlen(newLabel) > MAX_LABEL_NAME_LENGTH)
    {
        printf("Error line %d: label too long\n",currentLine);
        return 0;
    }

    /* Check if label is a reserved word */
    if(getInstruction(newLabel) >= 0 ||
        strcmp(newLabel,"ORG") == 0 || 
        strcmp(newLabel,"DUP") == 0 ||
        strcmp(newLabel,"DW") == 0 ||
        strcmp(newLabel,"DB") == 0 ||
        strcmp(newLabel,"NOP") == 0)
    {
        printf("Error line %d: reserved word %s found in column 1\n",currentLine,newLabel);
        return 0;
    }

    /* Check if label is already defined */
    for(i=0;i<numLabels;i++)
    {
        if(strcmp(labels[i].name,newLabel)==0)
        {
            printf("Error line %d: label %s already defined\n",currentLine,newLabel);
            return 0;
        }
    }

    /* Check if we have too many labels */
    if(numLabels >= MAX_LABELS)
    {
        printf("Error line %d: too many labels\n",currentLine);
        return 0;
    }

    /* Add label to list */
    strcpy(labels[numLabels].name,newLabel);
    labels[numLabels].value = currentAddress;
    numLabels++;
    return 1;
}

/*
    Check if the first word is a valid label then add to the label list checking it does not
    already exist. A label must start at column 1 and start with a alpha character and
    only contain alpha-numeric and underscore characters.
*/

int firstWordLabel(void)
{
    int i;
    char c;

    if(isalpha(line[0]))
    {
        for(i=0;i<MAX_LABEL_NAME_LENGTH;i++)
        {
            /* Check character by character */
            c = words[0][i];
            if(c == 0)
            {
                /* End of word - try to add to label list */
                if(!addLabel(words[0]))
                {
                    /* Error adding label, increment error counter and clear word count to stop parsing the rest of the line */
                    errorCount++;
                    wordCount = 0;
                    return 0;
                }
                else
                {
                    /* New label created */
                    return 1;
                }
            }
            if((!isalnum(c)) && c != '_')
            {
                break; /* Not a valid label */
            }
        }
    }
    return 0; /* Not a valid label */
}

/*
    Handle the ORG directive, change currentAddress to value following ORG
    firstWord indexes 'ORG' in the words array
*/

void parseORG(int firstWord)
{
    int orgValue;
    char *ptr;

    /* Check for correct number of words and parameter starts with a number */
    if((wordCount != firstWord + 2) || (!isdigit(words[firstWord+1][0])))
    {
        printf("Error line %d: ORG expects a single numeric value\n",currentLine);
        errorCount++;
        return;
    }

    /* Convert string to int */
    orgValue = (int)strtol(words[firstWord+1],&ptr,0);

    /* Check for garbage after number */
    if(*ptr != 0)
    {
        printf("Error line %d: syntax\n",currentLine);
        errorCount++;
        return;
    }

    /* Check if ORG is backwards */
    if(orgValue < currentAddress)
    {
        printf("Error line %d: ORG preceeds current address\n",currentLine);
        errorCount++;
        return;
    }

    /* Check ORG is within memory range */
    if(orgValue >= memorySize)
    {
        printf("Error line %d: ORG exceeds memory size\n",currentLine);
        errorCount++;
        return;  
    }

    /* Change current address to ORG value */
    currentAddress = orgValue;
}

/*
    Check if label exists, return label value or -1 if it does not exist.
*/

int findLabel(char *word)
{
    int i;

    for(i=0;i<numLabels;i++)
    {
        if(strcmp(labels[i].name,word)==0)
        {
            return labels[i].value;
        }
    }

    return -1;
}

/*
    Check and extract DUP from DW / DB
*/

int parseDUP(int firstWord)
{
    char *endStrol;

    /* Nothing to do if wordCount = firstword + 2 */
    if(wordCount == firstWord + 2)
    {
        dup = 0;
        return 1;
    }


    /* Check if line includes DUP */
    if(wordCount == firstWord + 4)
    {
        if(strcmp(words[firstWord+2],"DUP")==0)
        {
            dup = (int)strtol(words[firstWord+3],&endStrol,0);
            if(*endStrol==0)
            {
               if(dup > sizeof(buffer))
               {
                    printf("Error line %d: DUP exceeds maximum\n",currentLine);
                    errorCount++;
                    return 0;
               }
               return 1;
            }            
        }
    }

    printf("Error line %d: syntax\n",currentLine);
    errorCount++;
    return 0;
}

/*
    Lower level DB parsing

    Returns bufferIndex value which will be 0 if an error was found
*/

int parseDB2(char *word)
{
    char *ptr;
    char *endStrol;
    long value;

    bufferIndex = 0;
    ptr = word;
    while(1)
    {
        /* Check for number */
        if(isdigit(*ptr))
        {
            value = strtol(ptr,&endStrol,0);
            /* Check valid delimiters */
            if(*endStrol == 0 || *endStrol == ',')
            {
                /* Range check */
                if(value < 256)
                {
                    /* Store value and advance index if space in buffer */
                    if(bufferIndex < DB_DW_BUFFER_SIZE)
                    {
                        buffer[bufferIndex++] = (int)value;
                    }                    
                    if(*endStrol == 0)
                    {
                        break; /* Finished */
                    }
                    else
                    {                        
                        ptr = endStrol + 1; /* More data, update and advance pointer past comma */                        
                        continue;
                    }
                }
                else
                {
                    printf("Error line %d: DB value exceeds 255\n",currentLine);
                    bufferIndex = 0;
                    errorCount++;
                    break;
                }
            }
        }

        /* Check for string in quotes */
        if(*ptr = '\"')
        {
            do /* Copy string to buffer */
            {
                ptr++;
                if(*ptr == 0 || *ptr == '\"')
                {
                    break;
                }
                if(bufferIndex < DB_DW_BUFFER_SIZE)
                {
                    buffer[bufferIndex++] = (int)*ptr;
                }
            }while(1);
            /* Check string terminated correctly */
            if(*ptr == '\"')
            {
                ptr++;
                if(*ptr == ',')
                {
                    ptr++; /* More data */
                    continue;
                }
                if(*ptr == 0)
                {
                    break; /* No more data */
                }
            }
        }

        printf("Error line %d: syntax\n",currentLine);
        bufferIndex = 0;
        errorCount++;
        break;
    }

    return bufferIndex;
}

/*
    Handle NOP - just a branch to the next instruction
*/

void parseNOP(int firstWord)
{
    /* Make sure there is nothing else on ths line */
    if(firstWord + 1 != wordCount)
    {
        printf("Error line %d: NOP does not take parameters\n",currentLine);
        errorCount++;
        return;  
    }

    memoryImage[currentAddress] = 0xB000 + currentAddress + 1;
    currentAddress++;
}

/*
    Handle DB
*/

void parseDB(int firstWord)
{
    int i;

    /* Check if there is something else on this line */
    if(wordCount < firstWord + 2)
    {
        printf("Error line %d: DB expects one or more values\n",currentLine);
        errorCount++;
        return;
    }

    /* Check for DUP */
    if(!parseDUP(firstWord))
    {
        return; /* An error was found */
    }

    /* Parse the line */
    if(parseDB2(words[firstWord+1]) == 0)
    {
        return; /* Error */
    }

    /* Duplicate */
    if(dup > 0)
    {
        if(bufferIndex > 1)
        {
            printf("Error line %d: can only duplicate a single byte\n",currentLine);
            errorCount++;
            return;
        }
        for(i=1;i<dup;i++)
        {
            if(bufferIndex < DB_DW_BUFFER_SIZE)
            {
                buffer[bufferIndex] = buffer[0];
            }
            bufferIndex++;
        }
    }

    /* Check for data overflow */
    if(bufferIndex > DB_DW_BUFFER_SIZE)
    {
        printf("Error line %d: too much data for DB\n",currentLine);
        errorCount++;
        return;
    }

    /* Add pading byte if odd number of bytes */
    if(bufferIndex%2)
    {
        buffer[bufferIndex++] = 0;
    }

    /* Check data fits into memory */
    if(currentAddress + (bufferIndex/2) > memorySize)
    {
        printf("Error line %d: DB exceeds remaining memory\n",currentLine);
        errorCount++;
        return;  
    }

    /* Copy to memory image */
    for(i=0;i<bufferIndex;i+=2)
    {
        memoryImage[currentAddress++] = (buffer[i] << 8)|(buffer[i+1]);
    }
}

/*
    Lower level DW parsing

    Returns bufferIndex value which will be 0 if an error was found
*/

int parseDW2(char *word)
{
    char *ptr;
    char *endStrol;
    long value;
    char string[MAX_LABEL_NAME_LENGTH];
    int stringIndex;

    bufferIndex = 0;
    ptr = word;
    while(1)
    {
        /* Check for number */
        if(isdigit(*ptr))
        {
            value = strtol(ptr,&endStrol,0);
            /* Check valid delimiters */
            if(*endStrol == 0 || *endStrol == ',')
            {
                /* Range check */
                if(value < 65536L)
                {
                    /* Store value and advance index if space in buffer */
                    if(bufferIndex < DB_DW_BUFFER_SIZE)
                    {
                        buffer[bufferIndex] = (int)value;
                    }  
                    bufferIndex++;                  
                    if(*endStrol == 0)
                    {
                        break; /* Finished */
                    }
                    else
                    {                        
                        ptr = endStrol + 1; /* More data, update and advance pointer past comma */                        
                        continue;
                    }
                }
                else
                {
                    printf("Error line %d: DW value exceeds 65536\n",currentLine);
                    bufferIndex = 0;
                    errorCount++;
                    break;
                }
            }
        }
        /* Check for leter - start of label */
        if(isalpha(*ptr))            
        {
            /* Copy string */
            stringIndex = 0;
            while(1)
            {
                if(stringIndex < MAX_LABEL_NAME_LENGTH)
                {                                        
                    if(*ptr == 0 || *ptr == ',')
                    {
                        string[stringIndex] = 0;
                        break;  
                    }
                    string[stringIndex++] = *ptr;
                    ptr++;
                }
                else
                {
                    /* If string is too long, clear and break so error will be detected trying to resolve label */
                    string[0] = 0;
                    *ptr = 0;
                    break;
                }
            }
            /* String copied ptr points to termination character */            
            if(pass==1)                      
            {
                /* Pass 1 - just increment index */
                bufferIndex++;
            }
            else
            {
                /* Pass 2 - resolve label */
                if(bufferIndex < DB_DW_BUFFER_SIZE)
                {
                    value = (long)findLabel(string);
                    if(value < 0)
                    {
                        /* Could not find label */
                        printf("Error line %d: failed to resolve label\n",currentLine);
                        bufferIndex = 0;
                        errorCount++;
                        break;
                    }
                    else
                    {
                        buffer[bufferIndex] = (int)value;
                    }                    
                }
                bufferIndex++;
            }
            /* Check for more data on this line */
            if(*ptr == 0)
            {                            
                break; /* Finished */
            }
            else
            {
                ptr++; /* More data move past comma */
                continue; 
            }
                    
        }
        /* If not a letter or number then syntax error */
        printf("Error line %d: syntax\n",currentLine);
        bufferIndex = 0;
        errorCount++;
        break;
    }

    return bufferIndex;
}

/*
    Handle DW
*/

void parseDW(int firstWord)
{
    int i;

    /* Check if there is something else on this line */
    if(wordCount < firstWord + 2)
    {
        printf("Error line %d: DW expects one or more values\n",currentLine);
        errorCount++;
        return;
    }

    /* Check for DUP */
    if(!parseDUP(firstWord))
    {
        return; /* An error was found */
    }

    /* Parse the line */
    if(parseDW2(words[firstWord+1]) == 0)
    {
        return; /* Error */
    }

    /* Duplicate */
    if(dup > 0)
    {
        if(bufferIndex > 1)
        {
            printf("Error line %d: can only duplicate a single word\n",currentLine);
            errorCount++;
            return;
        }
        for(i=1;i<dup;i++)
        {
            if(bufferIndex < DB_DW_BUFFER_SIZE)
            {
                buffer[bufferIndex] = buffer[0];
            }
            bufferIndex++;
        }
    }

    /* Check for data overflow */
    if(bufferIndex > DB_DW_BUFFER_SIZE)
    {
        printf("Error line %d: too much data for DW\n",currentLine);
        errorCount++;
        return;
    }

    /* Check data fits into memory */
    if(currentAddress + bufferIndex > memorySize)
    {
        printf("Error line %d: DW exceeds remaining memory\n",currentLine);
        errorCount++;
        return;  
    }

    /* Copy to memory image */
    for(i=0;i<bufferIndex;i++)
    {
        memoryImage[currentAddress++] = buffer[i];
    }
}

/*
    Get immediate value from string
*/

int getImmediateValue(char *operand, int *value)
{
    char *endStrol;
    
    if(strlen(operand) > 0)
    {
        *value = (int)strtol(operand,&endStrol,0);
        return (*endStrol == 0);
    }
    return 0;
}

/*
    Add immediate to list if not already there and return address
*/

int resolveImmediate(int value)
{
    int i;

    /* Check if immediate has been used before */
    for(i=0;i<numImmediates;i++)
    {
        if(immediates[i].value == value)
        {
            /* Retrun address of existing immediate */
            return immediates[i].address;
        }
    }

    /* Create if space for new immediate */
    if(numImmediates >= MAX_IMMEDIATES)
    {
        printf("Error line %d: too many immediates\n",currentLine);
        errorCount++;
        return 0;
    }

    /* Create new immediate */
    immediates[numImmediates].value = value;
    immediates[numImmediates++].address = endAddress;

    /* Add to memory image if there is space - overflow error detected later on */
    if(endAddress < memorySize)
    {
        memoryImage[endAddress] = value;
    }
    
    return(endAddress++);
}


/*
    Parse instruction
*/

void parseInstruction(int instruction, int operandCount, char*operand)
{
    int value;

    /* Check correct number of operands first */
    if(instruction==15 && operandCount > 0)
    {
        printf("Error line %d: operand not valid for RETURN instruction\n",currentLine);
        errorCount++;
        return;
    }
    if(instruction!=15 && operandCount != 1)
    {
        printf("Error line %d: instruction expects an operand\n",currentLine);
        errorCount++;
        return;
    }
    /* Shift opcode */
    instruction <<= 12; 
    /* Handle operand if there is one */
    while(operandCount)
    {
        if(operand[0]=='#') /* Immediate */
        {
            if(getImmediateValue(&operand[1],&value))
            {
                instruction |= resolveImmediate(value);
                break;
            }
        }  
        if(operand[0]=='@') /* Address of label */
        {
            value = findLabel(&operand[1]);
            if(value >= 0)
            {
                instruction |= resolveImmediate(value);
                break;
            }
        }
        /* Any error with '#' or '@' will fall through here and findLable will fail */
        value = findLabel(operand);       
        if(value >= 0)
        {
            /* Label found */
            instruction |= value;
            break;
        }
        /* Else error ... */
        printf("Error line %d: syntax\n",currentLine);
        errorCount++;
        break;
    }

    memoryImage[currentAddress] = instruction;
}

/*
    Assemble a line of source code, pass variable controls flow
*/

void assembleLine(void)
{
    int thisWord = 0;
    int instruction;

    thisWord = 0;

    /* Interate twice if first word is a label */
    while(1)
    {
        /* Instruction? */
        instruction = getInstruction(words[thisWord]);
        if(instruction >= 0)
        {
            if(pass == 2)
            {      
                /* Parse instruction in second pass only */                                    
                parseInstruction(instruction,wordCount-thisWord-1,words[thisWord+1]);           
            }           
            currentAddress++;
            break;
        }
        /* ORG directive ? */
        if(strcmp("ORG",words[thisWord])==0)
        {
            parseORG(thisWord);
            break;;
        }
        /* DB */
        if(strcmp("DB",words[thisWord])==0)
        {
            parseDB(thisWord);
            break;
        }
        /* DW */
        if(strcmp("DW",words[thisWord])==0)
        {
            parseDW(thisWord);
            break;
        }
        /* NOP */
        if(strcmp("NOP",words[thisWord])==0)
        {
            parseNOP(thisWord);
            break;
        }
        /* Label? - only for first word */
        if(thisWord == 0)
        {
            if(pass == 1)
            {
                if(firstWordLabel())
                {
                    if(wordCount > 1)
                    {
                        thisWord = 1;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            if(pass == 2)
            {
                if(findLabel(words[0]) >= 0)
                {
                    if(wordCount > 1)
                    {
                        thisWord = 1;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }        
        printf("Error line %d: syntax\n",currentLine);
        errorCount++;
        break;
    }
}

/*
    Two pass assembler loop
*/

int assemble(FILE *fp)
{
    /* Get ready for first pass */
    numLabels = 0;
    numImmediates = 0;
    currentAddress = 0;
    errorCount = 0;
    currentLine = 1;    
    fseek(fp, 0, SEEK_SET);
    /* Parse each line - find labels, constants and basic syntax checking */
    pass = 1;
    printf("Pass 1\n");
    while(fgets(line,sizeof(line),fp))
    {
        splitLine();
        if(wordCount > 0)
        {
            assembleLine();
        }
        currentLine++;
    }
    if(errorCount == 0)
    {
        /* Get ready for second pass */
        endAddress = currentAddress; /* Used to add immediates to end of memory during pass 2 */
        fseek(fp, 0, SEEK_SET);
        currentLine = 1;
        currentAddress = 0;
        memset(memoryImage,0,sizeof(memoryImage));     
        /* Parse each line in file and create output memory image */   
        pass = 2;
        printf("Pass 2\n");
        while(fgets(line,sizeof(line),fp))
        {
            splitLine();
            if(wordCount > 0)
            {
                assembleLine();
            }
            currentLine++;
        }
    }

    /* Check program fits into our memory */
    if(endAddress > memorySize)
    {
        printf("Error: Program too big for memory\n");
        errorCount++;
    }

    /* Display assembly results */
    if(errorCount == 0)
    {
        printf("Assembly successfull %d memory words used\n",endAddress);
    }
    else
    {
        printf("Assembly failed with %d errors\n",errorCount);
    }

    /* Close source file */
    fclose(fp);

    return (errorCount==0);
}

void print_usage(void)
{
    printf("Usage:\n");
    printf("       pasm source.asm [S] output.(vhd|mem|mif)\n\n");
    printf("       Optional parameter S is the target memory size; a 2^n number\n");
    printf("       in the range 32 to 4096 defaults to 2048\n");
    printf("       The output file extention determines the output format:\n");
    printf("          .vhd  creates a VHLD initialized RAM model\n");
    printf("          .mif  creates a Intel/Altera MIF File\n");
    printf("          .mem  creates a Lattice Semiconductors MEM File\n");
}

int main(int argc, char *argv[])
{
    FILE *fp;
    char *endStrol;
    int outFileArg;
    char *outFileExtention;
    
    if(argc < 3 || argc > 4)
    {
        print_usage();
        return 0;
    }

    
    memorySize = 2048; /* Default Memory Size */
    outFileArg = 2;

    if(argc == 4)
    {
        memorySize = (int)strtol(argv[2],&endStrol,0);
        if(*endStrol != 0 || memorySize & (memorySize-1) != 0 | memorySize < 32 || memorySize > 4096)
        {
            print_usage();
            return 0;
        }
        outFileArg = 3;
    }

    fp = fopen(argv[1],"r");   

    if(fp == NULL)
    {
        printf("Could not open source file %s\n",argv[1]);
        return 0;
    }

    if(assemble(fp))
    {
        /* Get output file extention to determine file format */
        outFileExtention = getExtension(argv[outFileArg]);
        /* VHDL */
        if(strcmp(outFileExtention,"VHD") == 0 || strcmp(outFileExtention,"vhd") == 0)
        {
            createVHDLFile(argv[outFileArg]);
            return 0;
        }
        /* Intel/Altera MIF */
        if(strcmp(outFileExtention,"MIF") == 0 || strcmp(outFileExtention,"mif") == 0)
        {
            createMIFFile(argv[outFileArg]);
            return 0;
        }
        /* Lattice Semmiconductor MEM */
        if(strcmp(outFileExtention,"MEM") == 0 || strcmp(outFileExtention,"mem") == 0)
        {
            createMEMFile(argv[outFileArg]);
            return 0;
        }
        /* Error */
        printf("Invalid output file extention\n");
    }
        
    return 0;
}

/* End of File */

