#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "heap.cpp"

typedef uint32_t u32;
typedef unsigned char u8;

void printCode(u32 code, u8 codeLength)
{
    for (int i = codeLength - 1; i >= 0; i--)
    {
        if (((code >> i) & 1) == 1)
        {
            fprintf(stdout, "1");
        }
        else
        {
            fprintf(stdout, "0");
        }
    }
}

void labelCodes(HeapNode * root, u8 depth, u32 code)
{
    if(root != NULL)
    {
        root->code = code;
        root->codeLength = depth + 1;
        if(root->left)
        {
            labelCodes(root->left, depth + 1, (code << 1) | 0);
        }
        if(root->right)
        {
            labelCodes(root->right, depth + 1, (code << 1) | 1);
        }
    }
}

int main(int argc, char ** args)
{
    int opt;
    int dflag = 0; // decode flag
    char * outputFileName = NULL;
    char * inputFileName = NULL; 

    // Process the options
    while ((opt = getopt(argc, args, "do:")) != -1)
    {
        switch (opt)
        {
            case 'd':
                dflag = 1;
                break;
            case 'o':
                outputFileName = optarg;
                break;
            default:
                fprintf(stderr, "Usage: huff [-d] [-o <outputFileName>] <filename>\n");
                return EXIT_FAILURE;
        }
    }
    
    // default file names
    if (outputFileName == NULL)
    {
        if (dflag == 0)
        {
            outputFileName = (char *)"huff-encoded";
        }
        else
        {
            outputFileName = (char *)"huff-decoded";
        }
    }

    // Process the input filename
    if (optind < argc)
    {
        inputFileName = args[optind];  // remaining argument should be the input filename
    }
    else
    {
        fprintf(stderr, "Usage: huff [-d] [-o <outputFileName>] <filename>\n");
        return EXIT_FAILURE;
    }


    FILE * fp = fopen(inputFileName, "rb");

    
    if (fp == NULL)
    {
        fprintf(stderr, "Error reading file: %s\n", inputFileName);
        return EXIT_FAILURE;
    }

    if (dflag)
    {
        // Last byte used for second last byte padding bits
        fseek(fp, -1, SEEK_END);
        int lastByte = fgetc(fp);
        long fileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (fileSize < 8)
        {
            fprintf(stderr, "File size is too small to hold decode meta information");
            return EXIT_FAILURE;
        }

        char * file = (char *)malloc(fileSize);
        if (file == NULL)
        {
            perror("Memory allocation failed\n");
            return EXIT_FAILURE;
        }

        // Read the file into the buffer
        if (fread(file, fileSize, 1, fp) != 1)
        {
            perror("Failed to read file\n");
            return EXIT_FAILURE;
        }

        file[fileSize] = '\0';

        u32 numOfTableChars = 0;
        long originalFileSize = 0; // needs to be 'long' since ftell on encoding returns a long which may be variable byte length depending on the system

        memcpy(&numOfTableChars, file, 4);
        memcpy(&originalFileSize, file + 4, 4);
        
        u32 bytesProcessed = 0;

        char * chs = (char *)malloc(numOfTableChars * sizeof(char));
        u8 * codeLengths = (u8 *)malloc(numOfTableChars * sizeof(u8));
        u32 * codes = (u32 *)malloc(numOfTableChars * sizeof(u32));

        if (chs == NULL || codeLengths == NULL || codes == NULL)
        {
            perror("Memory allocation failed");
            return EXIT_FAILURE;
        }

        u32 initialOffset = 4 + sizeof(long); // 4 bytes for numOfTableChars + 'long' number of bytes for originalFileSize
        u32 blockSize = 1 + 1 + 4; // 1 byte for the character, 1 byte for its code length, 4 bytes for the code

        for (u32 i = 0; i < numOfTableChars; i++) {
            u32 blockStart = initialOffset + i * blockSize;

            char c;
            memcpy(&c, file + blockStart, 1);
            chs[i] = c;

            u8 codeLength;
            memcpy(&codeLength, file + blockStart + 1, 1);
            codeLengths[i] = codeLength;

            u32 code;
            memcpy(&code, file + blockStart + 1 + 1, 4);
            codes[i] = code;
#ifdef DEBUG
            fprintf(stdout, "ch = %c codeLength = %d code = ", c, codeLength);
            printCode(code, codeLength);
            fprintf(stdout, "\n");
#endif
        }

        bytesProcessed = initialOffset + numOfTableChars * blockSize;

        // Decode the remaining bytes
        u8 *decoded = (u8 *)malloc(originalFileSize * sizeof(u8));
        u32 pos = 0;
        u32 currentCode = 0;
        u8 currentCodeLength = 0;

        initialOffset = bytesProcessed;

        // Calculate remaining bytes to process, account for last byte (padding information)
        long leftOverBytes = fileSize - bytesProcessed - 1;
        for (long b = 0; b < leftOverBytes; b++)
        {
            u8 c;
            memcpy(&c, file + initialOffset + b, 1);

            // Handle edge case for last coded byte that may contain padding
            int bitsToProcess = 8;
            if (b == leftOverBytes - 1 && lastByte > 0) // lastByte == 0 when bits pack perfectly into bytes
            {
                bitsToProcess = lastByte;
            }

            for (int i = 0; i < bitsToProcess; i++)
            {
                u32 bit = (c >> (7 - i)) & 1;
                currentCode = (currentCode << 1) | bit;
                currentCodeLength++;

#ifdef DEBUG
                fprintf(stdout, "currentCode = ");
                printCode(currentCode, currentCodeLength);
                fprintf(stdout, "\n");
#endif

                // Check if currentCode matches any of the table codes
                for (int n = 0; n < numOfTableChars; n++)
                {
                    if (currentCodeLength == codeLengths[n] && currentCode == codes[n])
                    {
                        decoded[pos++] = chs[n];
                        currentCode = 0;
                        currentCodeLength = 0;
                        break;
                    }
                }
            }
        }

        // Write to output file
        FILE * outputFile = fopen(outputFileName, "wb");

        if (outputFile == NULL)
        {
            fprintf(stderr, "Error opening file %s\n", outputFileName);
            return EXIT_FAILURE;
        }

        fwrite(decoded, 1, pos, outputFile);
    }
    else
    {

        fseek(fp, -1, SEEK_END);
        long fileIndex = ftell(fp);
        long fileSize = fileIndex + 1;
        fseek(fp, 0, SEEK_SET);

        char * file = (char *)malloc(fileSize + 1);
        
        if (file == NULL)
        {
            perror("Memory allocation failed\n");
            return EXIT_FAILURE;
        }
        
        if (fread(file, fileSize, 1, fp) != 1)
        {
            perror("Failed to read file\n");
            return EXIT_FAILURE;
        }
        
        file[fileSize] = '\0';

        char * chs = (char *)calloc(fileSize, sizeof(char));
        u32 * occ = (u32 *)calloc(fileSize, sizeof(u32));

        if (chs == NULL || occ == NULL)
        {
            perror("Memory allocation failed\n");
            return EXIT_FAILURE;
        }

        u32 size = 0;
        for (long i = 0; i < fileSize; ++i)
        {
            for (long j = 0; j < fileSize; j++)
            {
                if (chs[j] == '\0')
                {
                    chs[j] = file[i];
                    size++;
                    occ[j] += 1;
                    break;
                }
                else if (chs[j] == file[i])
                {
                    occ[j] += 1;
                    break;
                }
            }
        }

        // min heap used here as a priority queue for building the huffman tree
        Heap * heap = (Heap *)malloc(sizeof(Heap));
        
        if (heap == NULL)
        {
            perror("Memory allocation failed\n");
            return EXIT_FAILURE;
        }

        heap->capacity = size;
        heap->size = 0;
        heap->array = (HeapNode **)malloc(heap->capacity * sizeof(HeapNode *));
        
        if (heap->array == NULL)
        {
            perror("Memory allocation failed\n");
            return EXIT_FAILURE;
        }

        //have array of leaves to use for later
        HeapNode ** leaves = (HeapNode **)malloc(heap->capacity * sizeof(HeapNode *));

        if (leaves == NULL)
        {
            perror("Memory allocation failed\n");
            return EXIT_FAILURE;
        }
        
        for (u32 i = 0; i < size; i++)
        {
            HeapNode * node = (HeapNode *)malloc(sizeof(HeapNode));

            if (node == NULL)
            {
                perror("Memory allocation failed\n");
                return EXIT_FAILURE;
            }

            node->ch = chs[i];
            node->freq = occ[i];
            leaves[i] = node;
            insertHeap(heap, node);

#ifdef DEBUG
            fprintf(stdout, "ch = %c freq = %d\n", node->ch, node->freq);
#endif
        }


        // combining always reduces the size of the heap by 1. we exit when we have 1 node in the heap, the root of the huffman tree.
        while(heap->size > 1)
        {
            HeapNode * first = extractMin(heap);
            HeapNode * second = extractMin(heap);
            HeapNode * combined = (HeapNode *)malloc(sizeof(HeapNode));
            
            if (combined == NULL)
            {
                perror("Memory allocation failed\n");
                return EXIT_FAILURE;
            }

            combined->freq = first->freq + second->freq;
            combined->left = first;
            combined->right = second;
            insertHeap(heap, combined);
        }

        HeapNode * huffmanRoot = heap->array[0];
        labelCodes(huffmanRoot->left, 0, 0);
        labelCodes(huffmanRoot->right, 0, 1);
        
        FILE * outputFile = fopen(outputFileName, "wb");
        fwrite(&size, sizeof(size), 1, outputFile); // write out number of characters so when decoding, know how many codes to parse
        fwrite(&fileSize, sizeof(fileSize), 1, outputFile); // write out original file size so we can know size of output data when decoding
        for (int i = 0; i < size; i++)
        {
            fwrite(&leaves[i]->ch, 1, 1, outputFile);
            fwrite(&leaves[i]->codeLength, 1, 1, outputFile);
            fwrite(&leaves[i]->code, 4, 1, outputFile);

#ifdef DEBUG
            fprintf(stdout, "ch = %c freq = %d codeLength = %d code = ", leaves[i]->ch, leaves[i]->freq, leaves[i]->codeLength);
            printCode(leaves[i]->code, leaves[i]->codeLength);
            fprintf(stdout, "\n");
#endif
        }

        u8 buffer = 0;
        u8 bitsInBuffer = 0;
        for (long i = 0; i < fileSize; i++) // TODO: use a map if size is large
        {
            for (u32 j = 0; j < size; j++)
            {
                if (file[i] == leaves[j]->ch)
                {
                    //pack code into bytes
                    u32 code = leaves[j]->code;
                    u8 codeLength = leaves[j]->codeLength;
                    for (int c = codeLength - 1; c >= 0; c--)
                    {
                        buffer = (buffer << 1) | ((code >> c) & 1);
                        bitsInBuffer++;
                        if (bitsInBuffer == 8)
                        {
                            fwrite(&buffer, 1, 1, outputFile);
                            buffer = 0;
                            bitsInBuffer = 0;
                        }
                    }
                    break; // once matched no need to check other leaves
                }
            }
        }

        if (bitsInBuffer > 0) {
            buffer <<= (8 - bitsInBuffer); // Adjust buffer since bits inserted from the right, but we will be reading bits from left to right
            fwrite(&buffer, 1, 1, outputFile);
        }

        // Write the number of valid bits in the last byte
        // To keep byte reading consistent also write out 0 for the last byte if bits fit perfectly into bytes, it will be ignored when decoding
        fwrite(&bitsInBuffer, 1, 1, outputFile);
    }
    
    return 0;
}