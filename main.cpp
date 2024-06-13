#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
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

void printCode2(u32 code, u8 codeLength)
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
        // fprintf(stdout, "ch = %c freq = %d code = ", root->ch, root->freq);
        root->code = code;
        root->codeLength = depth + 1;
        // printCode(root);
        // fprintf(stdout, "\n");
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
        
        FILE * fp = fopen(filename, "rb");
        
        if (fp != NULL)
        {

            if (decode)
            {
                // Move to the end to read the last byte for padding information
                fseek(fp, -1, SEEK_END);
                int lastByte = fgetc(fp);
                long fileSize = ftell(fp);
                fseek(fp, 0, SEEK_SET);

                char * file = (char *)malloc(fileSize + 1);
                if (file == NULL) {
                    perror("Memory allocation failed");
                    fclose(fp);
                    return EXIT_FAILURE;
                }

                // Read the file into the buffer
                if (fread(file, fileSize, 1, fp) != 1) {
                    perror("Failed to read file");
                    free(file);
                    fclose(fp);
                    return EXIT_FAILURE;
                }

                file[fileSize] = '\0';

                u32 numOfChars = 0;
                long sizeOfDecoded = 0;
                u32 bytesProcessed = 0;

                if (fileSize >= 8) {
                    memcpy(&numOfChars, file, 4);
                    memcpy(&sizeOfDecoded, file + 4, 4);
                } else {
                    perror("File size is less than 8 bytes, cannot decode numOfChars and sizeOfDecoded\n");
                    free(file);
                    fclose(fp);
                    return EXIT_FAILURE;
                }


                char *chs = (char *)malloc(numOfChars * sizeof(char));
                u8 *codeLengths = (u8 *)malloc(numOfChars * sizeof(u8));
                u32 *codes = (u32 *)malloc(numOfChars * sizeof(u32));

                u32 offset = 4 + sizeof(sizeOfDecoded); // 4 bytes for numOfChars + 'long' number of bytes for sizeOfDecoded
                bytesProcessed = offset;
                u32 blockSize = 1 + 1 + 4; // 1 byte for char, 1 byte for codeLength, 4 bytes for code

                for (u32 i = 0; i < numOfChars; i++) {
                    u32 byteOffset = offset + i * blockSize;
                    assert(byteOffset + blockSize < fileSize);

                    char c;
                    memcpy(&c, file + byteOffset, 1);
                    chs[i] = c;

                    u8 codeLength;
                    memcpy(&codeLength, file + byteOffset + 1, 1);
                    codeLengths[i] = codeLength;

                    u32 code;
                    memcpy(&code, file + byteOffset + 1 + 1, 4);
                    codes[i] = code;

                    fprintf(stdout, "ch = %c codeLength = %d code = ", c, codeLength);
                    printCode2(code, codeLength);
                    fprintf(stdout, "\n");
                }
                bytesProcessed += numOfChars * blockSize;

                // Decode the remaining bytes
                u8 *decoded = (u8 *)malloc(sizeOfDecoded * sizeof(u8));
                u32 pos = 0;
                u32 currentCode = 0;
                u8 currentCodeLength = 0;

                offset = bytesProcessed;

                // Calculate remaining bytes to process, account for last byte (padding information)
                long leftOverBytes = fileSize - bytesProcessed - 1;
                for (long b = 0; b < leftOverBytes; b++) {
                    u8 c;
                    memcpy(&c, file + offset + b, 1);

                    // Handle edge case for last coded byte that may contain padding
                    int bitsToProcess = 8;
                    if (b == leftOverBytes - 1) {
                        bitsToProcess = lastByte;
                    }

                    for (int i = bitsToProcess - 1; i >= 0; i--) {
                        u32 bit = (c >> i) & 1;
                        currentCode = (currentCode << 1) | bit;
                        currentCodeLength++;
                        // fprintf(stdout, "currentCode = ");
                        // printCode2(currentCode, currentCodeLength);
                        // fprintf(stdout, "\n");

                        assert(currentCodeLength <= 32);

                        // Check if currentCode matches any of the codes
                        for (int n = 0; n < numOfChars; n++) {
                            if (currentCodeLength == codeLengths[n] && currentCode == codes[n]) {
                                // fprintf(stdout, "ch = %c pos = %d\n", chs[n], pos);
                                decoded[pos++] = chs[n];
                                assert(pos <= sizeOfDecoded);
                                currentCode = 0;
                                currentCodeLength = 0;
                                break;
                            }
                        }
                    }
                }

                // Write to output file
                FILE *outputFile = fopen("output2.txt", "wb");
                fwrite(decoded, 1, pos, outputFile);
            }
            else
            {

                fseek(fp, -1, SEEK_END);
                long fileSize = ftell(fp);
                fseek(fp, 0, SEEK_SET);

                char * file = (char *)malloc(fileSize + 1);
                fread(file, fileSize, 1, fp);
                file[fileSize] = '\0';

                char * chs = (char *)calloc(fileSize, sizeof(char));
                u32 * occ = (u32 *)calloc(fileSize, sizeof(u32));
                u32 size = 0;
                if (chs != NULL && occ != NULL)
                {
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
                    heap->capacity = size;
                    heap->size = 0;
                    heap->array = (HeapNode **)malloc(heap->capacity * sizeof(HeapNode *));
                    //have array of leaves to use for later
                    HeapNode ** leaves = (HeapNode **)malloc(heap->capacity * sizeof(HeapNode *));
                    
                    for (u32 i = 0; i < size; i++)
                    {
                        HeapNode * node = (HeapNode *)malloc(sizeof(HeapNode));
                        node->ch = chs[i];
                        node->freq = occ[i];
                        leaves[i] = node;
                        insertHeap(heap, node);
                        fprintf(stdout, "ch = %c freq = %d\n", node->ch, node->freq);
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
                    fwrite(&fileSize, sizeof(fileSize), 1, outputFile); // write out original file size so we can know size of output data when decoding
                    for (int i = 0; i < size; i++)
                    {
                        fwrite(&leaves[i]->ch, 1, 1, outputFile);
                        fwrite(&leaves[i]->codeLength, 1, 1, outputFile);
                        fwrite(&leaves[i]->code, 4, 1, outputFile);
                        fprintf(stdout, "ch = %c freq = %d codeLength = %d code = ", leaves[i]->ch, leaves[i]->freq, leaves[i]->codeLength);
                        printCode(leaves[i]);
                        fprintf(stdout, "\n");
                    }

                    u8 buffer = 0;
                    u8 bits_in_buffer = 0;
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