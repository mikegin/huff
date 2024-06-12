#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "heap.cpp"

typedef uint32_t u32;
typedef unsigned char u8;

void printCode(HeapNode * node)
{
    u32 code = node->code;
    for (int i = node->codeLength - 1; i >= 0; i--)
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

void printCode2(u32 code, u32 codeLength)
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

void labelCodes(HeapNode * root, u32 depth, u32 code)
{
    if(root != NULL)
    {
        fprintf(stdout, "ch = %c freq = %d code = ", root->ch, root->freq);
        root->code = code;
        root->codeLength = depth + 1;
        printCode(root);
        fprintf(stdout, "\n");
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

void verifyHeap(HeapNode * root)
{
    if (root != NULL)
    {
        
        if (root->left && root->right)
        {
            assert(root->freq == root->left->freq + root->right->freq);
            
        }
        else if (!root->left && !root->right)
        {
            fprintf(stdout, "ch = %c freq = %d code = ");
            printCode(root);
            fprintf(stdout, "\n");
        }
        else
        {
            assert(false); // should not have just 1 child;
        }
        verifyHeap(root->left);
        verifyHeap(root->right);
    }
}

int main(int argc, char ** args)
{
    if (argc > 1)
    {
        int decode = 0;
        char * filename;

        if (argc > 2)
        {
            if (strcmp(args[1], "-d") == 0)
            {
                decode = 1;
                filename = args[2];
            }
            else
            {
                fprintf(stderr, "Usage: huff [-d] <filename>\n");
                return -1;    
            }
        }
        else
        {
            filename = args[1];
        }
        
        FILE * fp = fopen(filename, "r");
        
        if (fp != NULL)
        {

            if (decode)
            {
                // last byte indicates how many bits used in last byte for actual coding vs padding
                fseek(fp, -1, SEEK_END);
                int lastByte = fgetc(fp);
                long fileSize = ftell(fp);
                fseek(fp, 0, SEEK_SET);

                // int ch;
                u32 numOfChars = 0; // 4287954944 for output.bin, 1419051008 if not doing fseek??
                u32 sizeOfDecoded = 0; // 6817024 for output.bin, 3295453
                u32 bytesProcessed = 0;
                // // first 4 bytes is number of characters to 
                // for (int i = 0; i < 4; i++)
                // {
                //     ch = fgetc(fp);
                //     if (ch == EOF)
                //     {
                //         fprintf(stderr, "Error decoding file, incorrect number of bytes needed to specify size of codes table.");
                //         return -1;
                //     }
                //     numOfChars = (numOfChars << 8) | (u8) ch;
                // }
                // for (int i = 0; i < 4; i++) {
                //     ch = fgetc(fp);
                //     if (ch == EOF) {
                //         fprintf(stderr, "Error decoding file, incorrect number of bytes needed to specify size of codes table.");
                //         fclose(fp);
                //         return -1;
                //     }
                //     numOfChars |= ((uint32_t)ch << (8 * i)); // Shift ch left by i*8 bits and combine it
                // }
                size_t result = fread(&numOfChars, sizeof(u32), 1, fp);
                if (result != 1) {
                    if (feof(fp)) {
                        printf("Error: Unexpected end of file.\n");
                    } else if (ferror(fp)) {
                        perror("Error reading from file");
                    }
                    fclose(fp);
                    return -1;
                }
                bytesProcessed += 4;

                result = fread(&sizeOfDecoded, sizeof(u32), 1, fp);
                if (result != 1) {
                    if (feof(fp)) {
                        printf("Error: Unexpected end of file.\n");
                    } else if (ferror(fp)) {
                        perror("Error reading from file");
                    }
                    fclose(fp);
                    return -1;
                }
                bytesProcessed += 4;

                u8 * chs = (u8 *)malloc(numOfChars * sizeof(u8));
                u32 * codeLengths = (u32 *)malloc(numOfChars * sizeof(u32));
                u32 * codes = (u32 *)malloc(numOfChars * sizeof(u32));
                for (u32 i = 0; i < numOfChars; i++)
                {
                    u8 c;
                    result = fread(&c, sizeof(c), 1, fp);
                    if (result != 1) {
                        if (feof(fp)) {
                            printf("Error: Unexpected end of file.\n");
                        } else if (ferror(fp)) {
                            perror("Error reading from file");
                        }
                        fclose(fp);
                        return -1;
                    }
                    chs[i] = c;

                    u32 codeLength = 0;
                    result = fread(&codeLength, sizeof(codeLength), 1, fp);
                    if (result != 1) {
                        if (feof(fp)) {
                            printf("Error: Unexpected end of file.\n");
                        } else if (ferror(fp)) {
                            perror("Error reading from file");
                        }
                        fclose(fp);
                        return -1;
                    }
                    codeLengths[i] = codeLength;

                    u32 code = 0;
                    result = fread(&code, sizeof(code), 1, fp);
                    if (result != 1) {
                        if (feof(fp)) {
                            printf("Error: Unexpected end of file.\n");
                        } else if (ferror(fp)) {
                            perror("Error reading from file");
                        }
                        fclose(fp);
                        return -1;
                    }
                    codes[i] = code;
                }
                bytesProcessed += numOfChars * (1 + 4 + 4);

                for (int i = 0; i < numOfChars; i++)
                {
                    fprintf(stdout, "ch = %c code = ", chs[i]);
                    u32 code = codes[i];
                    u32 codeLength = codeLengths[i];
                    printCode2(code, codeLength);
                    if (code == 0)
                    {
                        fprintf(stdout, " is zero");
                    }
                    fprintf(stdout, "\n");
                }

                // parse till end of file
                u8 * decoded = (u8 *)malloc(sizeOfDecoded * sizeof(u8));
                u32 pos = 0;
                u32 currentCode = 0;
                int currentCodeLength = 0;
                // while ((ch = fgetc(fp)) != EOF)
                int leftOverBytes = fileSize - bytesProcessed - 1; // last byte used 
                for (int b = 0; b < leftOverBytes; b++)
                {
                    u8 c;
                    result = fread(&c, sizeof(c), 1, fp);
                    if (result != 1) {
                        if (feof(fp)) {
                            printf("Error: Unexpected end of file.\n");
                        } else if (ferror(fp)) {
                            perror("Error reading from file");
                        }
                        fclose(fp);
                        return -1;
                    }

                    int i = 7;

                    // handle edge case where last coded byte may not have a full code and have padding zeroes
                    if (b == leftOverBytes - 1) // second last byte
                    {
                        i = lastByte - 1;
                    }

                    for (; i >= 0 ; i--)
                    {
                        // int bit = (c >> i) & 1;
                        u32 bit = ((u32)c >> i) & 1;
                        currentCode <<= 1;
                        currentCode |= bit;
                        currentCodeLength++;

                        fprintf(stdout, "currentCode = ");
                        printCode2(currentCode, currentCodeLength);
                        fprintf(stdout, "\n");

                        assert(currentCodeLength <= 32);
                        // check if currentCode matches a code
                        for (int n = 0; n < numOfChars; n++)
                        {
                            u32 code = codes[n];
                            u32 codeLength = codeLengths[n];
                            if (currentCodeLength == codeLength && currentCode == code)
                            {
                                char ch_ = chs[n];
                                decoded[pos] = ch_;
                                pos++;
                                fprintf(stdout, "ch = %c pos = %d\n", ch_, pos);
                                // reset currentCode
                                currentCode = 0; 
                                currentCodeLength = 0;
                            }
                        }
                    }
                }
                
                FILE * outputFile = fopen("output2.txt", "w");
                fwrite(&decoded, 1, pos, outputFile);
            }
            else
            {
                int len = 4096;
                u8 * chs = (u8 *)calloc(len, sizeof(char));
                int * occ = (int *)calloc(len, sizeof(int));
                u32 sizeOfFile = 0;
                if (chs != NULL && occ != NULL)
                {
                    int ch; // use int to capture all values including EOF
                    while ((ch = fgetc(fp)) != EOF)
                    {
                        sizeOfFile += 1;

                        for (int i = 0; i < len; ++i)
                        {
                            if (chs[i] == '\0')
                            {
                                chs[i] = (u8) ch;
                                occ[i] += 1;
                                break;
                            }
                            else if (chs[i] == ch)
                            {
                                occ[i] += 1;
                                break;
                            }
                        }
                    }

                    u32 size = 0;
                    u8 * t = chs;
                    while(*t++ != '\0')
                    {
                        size++;
                    }

                    // min heap used here as a priority queue for building the huffman tree
                    Heap * heap = (Heap *)malloc(sizeof(Heap));
                    heap->capacity = size;
                    heap->size = 0;
                    heap->array = (HeapNode **)malloc(heap->capacity * sizeof(HeapNode *));
                    //have array of leaves to use for later
                    HeapNode ** leaves = (HeapNode **)malloc(heap->capacity * sizeof(HeapNode *));
                    
                    for (int i = 0; i < size; i++)
                    {
                        HeapNode * node = (HeapNode *)malloc(sizeof(HeapNode));
                        node->ch = chs[i];
                        node->freq = occ[i];
                        leaves[i] = node;
                        insertHeap(heap, node);
                    }

                    // combining always reduces the size of the heap by 1. we exit when we have 1 node in the heap, the root of the huffman tree.
                    while(heap->size > 1)
                    {
                        HeapNode * first = extractMin(heap);
                        HeapNode * second = extractMin(heap);
                        HeapNode * combined = (HeapNode *)malloc(sizeof(HeapNode));
                        combined->freq = first->freq + second->freq;
                        combined->left = first;
                        combined->right = second;
                        insertHeap(heap, combined);
                    }

                    assert(heap->size == 1);
                    // verifyHeap(heap->array[0]);

                    HeapNode * huffmanRoot = heap->array[0];
                    labelCodes(huffmanRoot->left, 0, 0);
                    labelCodes(huffmanRoot->right, 0, 1);
                    
                    FILE * outputFile = fopen("output.bin", "wb");
                    fwrite(&size, sizeof(size), 1, outputFile); // write out number of characters so when decoding, know how many codes to parse
                    fwrite(&sizeOfFile, sizeof(sizeOfFile), 1, outputFile); // write out size of file so we can know size of what output should be when decoding
                    for (int i = 0; i < size; i++)
                    {
                        // fprintf(stdout, "ch = %c freq = %d code = ", leaves[i]->ch, leaves[i]->freq);
                        // printCode(leaves[i]);
                        // fprintf(stdout, "\n");
                        fwrite(&leaves[i]->ch, sizeof(leaves[i]->ch), 1, outputFile);
                        fwrite(&leaves[i]->codeLength, sizeof(leaves[i]->codeLength), 1, outputFile);
                        fwrite(&leaves[i]->code, sizeof(leaves[i]->code), 1, outputFile);
                    }

                    fseek(fp, 0, SEEK_SET);
                    u8 buffer = 0;
                    u8 bits_in_buffer = 0;
                    while ((ch = fgetc(fp)) != EOF) {
                        for (int i = 0; i < size; i++) // TODO: use a map if size is large
                        {
                            if (ch == leaves[i]->ch)
                            {
                                //pack code into bytes
                                for (int c = 0; c < leaves[i]->codeLength; c++)
                                {
                                    buffer = (buffer << 1) | ((leaves[i]->code >> c) & 1);
                                    bits_in_buffer++;
                                    if (bits_in_buffer == 8)
                                    {
                                        fwrite(&buffer, 1, 1, outputFile);
                                        buffer = 0;
                                        bits_in_buffer = 0;
                                    }
                                }
                                break; // once matched no need to check other leaves
                            }
                        }
                    }

                    if (bits_in_buffer > 0) {
                        buffer <<= (8 - bits_in_buffer); // Shift to left since will be reading bits from left to right
                        fwrite(&buffer, 1, 1, outputFile);
                        // Write the number of valid bits in the last byte
                        fwrite(&bits_in_buffer, 1, 1, outputFile);
                    }

                }
                else
                {
                    fprintf(stderr, "Memory allocation failed \n");    
                }
            }
                
        }
        else
        {
            fprintf(stderr, "Error reading file: %s\n", filename);
        }
    }
    else
    {
        fprintf(stderr, "Usage: huff [-d] <filename>\n");
    }

    return 0;
}