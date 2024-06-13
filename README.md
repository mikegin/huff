# huff

## A compression tool using Huffman Coding

### compile
```
clang++ main.cpp -o huff -O3
```

### run
```
./huff -o huff.encoded les-miserable.txt

./huff -o huff.decoded huff.encoded
```

### verify

that the input was compressed:
```
ls -l les-miserable.txt
ls -l huff.encoded 
```

uncompressed data is the same as the original:
```
diff huff.decoded les-miserable.txt
```