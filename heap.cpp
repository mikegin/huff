#include <stdio.h>
#include <stdint.h>

typedef uint32_t u32;

typedef struct HeapNode {
    char ch;
    u32 freq;
    struct HeapNode *left, *right;
    u32 code;
    u32 codeLength;
} HeapNode;

typedef struct Heap {
    u32 size; // current size
    u32 capacity; // overall size
    struct HeapNode ** array;
} Heap;

void swap(HeapNode **a, HeapNode **b) {
    HeapNode *t = *a;
    *a = *b;
    *b = t;
}

void heapify(Heap * heap, u32 idx)
{
    u32 smallest = idx;
    u32 left = 2 * idx + 1;
    u32 right = 2 * idx + 2;        

    if (left < heap->size && heap->array[left]->freq < heap->array[smallest]->freq)
        smallest = left;
    if (right < heap->size && heap->array[right]->freq < heap->array[smallest]->freq)
        smallest = right;
    
    if (smallest != idx)
    {
        swap(&heap->array[smallest], &heap->array[idx]);
        heapify(heap, smallest);
    }
}

void insertHeap(Heap * heap, HeapNode * node)
{
    if (heap->size == heap->capacity)
    {
        printf("Heap size is at capacity. Nothing inserted.");
        return;
    }

    heap->size++;
    int i = heap->size - 1;
    heap->array[i] = node;

    while(i > 0 && heap->array[(i - 1)/2]->freq > heap->array[i]->freq)
    {
        swap(&heap->array[(i - 1)/2], &heap->array[i]);
        i = (i - 1) / 2;
    }
}

HeapNode * extractMin(Heap * heap)
{
    if (heap->size <= 0)
        return NULL;

    HeapNode * root = heap->array[0];
    heap->array[0] = heap->array[heap->size - 1];
    heap->size--;
    heapify(heap, 0);

    return root;
}