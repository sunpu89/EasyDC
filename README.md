# EasyDC: A Domain-Specific Language for Security Analysis of Block Ciphers against Differential Cryptanalysis

This repository is for parsing the input **[EasyDC](https://github.com/GOODMORNING616/EasyDC)** programs of block ciphers and then performing the security analysis of block ciphers against differential cryptanalysis by solving MILP instances generated from the preprocessed programs of inputs.
It also support the modelings of S-boxes separately, which generate IL constraints of DDTs of the given S-boxes.

## Preparatory Environment:
### 1. [flex](https://github.com/westes/flex), [bison](https://www.gnu.org/software/bison/) and [jsoncpp](https://github.com/open-source-parsers/jsoncpp) : 
used for **EasyDC** language and parser for preprocessing.
### 2. [Gurobi](https://www.gurobi.com/solutions/gurobi-optimizer/) : 
used for solving MILP instances which characterize the relation between input and output differences of initial input programs. 
### 3. [Z3](https://github.com/Z3Prover/z3) : 
used for computing branch number of various operators, checking if a given S-box is injective, minimizing the number of Boolean variables used for encoding the probabilities of differential propagations in DDTs (MaxSMT).
<!-- ### 3. [flex](https://github.com/westes/flex) -->
<!-- ### 4. [bison](https://www.gnu.org/software/bison/) -->
<!-- ### 5. [jsoncpp](https://github.com/open-source-parsers/jsoncpp) -->

## Project Structure
- [root](https://github.com/easydcpro/easydc)
  - [benchmarks](https://github.com/GOODMORNING616/EasyDC/tree/main/benchmarks)
    - [BlockCipher](https://github.com/GOODMORNING616/EasyDC/tree/main/benchmarks/BlockCipher)
    - [NIST](https://github.com/GOODMORNING616/EasyDC/tree/main/benchmarks/NIST)
    - [WordWise](https://github.com/GOODMORNING616/EasyDC/tree/main/benchmarks/WordWise)   
  - [data](https://github.com/GOODMORNING616/EasyDC/tree/main/data)
    - [differential](https://github.com/GOODMORNING616/EasyDC/tree/main/data/differential)
      - [BitWiseMILP](https://github.com/GOODMORNING616/EasyDC/tree/main/data/differential/BitWiseMILP)
        - [dc](https://github.com/GOODMORNING616/EasyDC/tree/main/data/differential/BitWiseMILP/dc) : the saved differential characteristics of bit-wise MILP instances and extended bit-wise MILP instances.
        - [models](https://github.com/GOODMORNING616/EasyDC/tree/main/data/differential/BitWiseMILP/models) : the constructed bit-wise and extended bit-wise MILP instances.
        - [results](https://github.com/GOODMORNING616/EasyDC/tree/main/data/differential/BitWiseMILP) : the solving results of bit-wise and extended bit-wise MILP instances.
      - [WordWiseMILP](https://github.com/GOODMORNING616/EasyDC/tree/main/data/differential/WordWiseMILP) 
        - [models](https://github.com/GOODMORNING616/EasyDC/tree/main/data/differential/WordWiseMILP/models) : the constructed word-wise MILP instances.
        - [results](https://github.com/GOODMORNING616/EasyDC/tree/main/data/differential/WordWiseMILP) : the solving results of word-wise MILP instances.
    - [sbox](https://github.com/GOODMORNING616/EasyDC/tree/main/data/sbox)
      - [ARX](https://github.com/GOODMORNING616/EasyDC/tree/main/data/sbox/ARX) : the modelings and results of arithmetic addition.
      - [AS](https://github.com/GOODMORNING616/EasyDC/tree/main/data/sbox/AS) : the modelings and results of look-up table S-boxes without the probabilities of possible differential propagations in DDTs.
      - [DC](https://github.com/GOODMORNING616/EasyDC/tree/main/data/sbox/DC) : the modelings and results of look-up table S-boxes with the probabilities of possible differential propagations in DDTs.
  - [include](https://github.com/GOODMORNING616/EasyDC/tree/main/include) : head files.
  - [lib](https://github.com/GOODMORNING616/EasyDC/tree/main/lib) : source files.
  - [main.cpp](https://github.com/GOODMORNING616/EasyDC/blob/main/main.cpp)


## Usage:
### 1. Modeling possible differential propagations in DDTs of S-boxes
  - comand : "./EasyDC argv[1]  argv[2]  argv[3]  argv[4] " : 

    |  | argv[1] | argv[2] | argv[3] | argv[4] |
    | :-----: | :-----: | :----: | :----: | :----: |
    | Remark | name of S-box | S-box | mode of modelings, taking probabilities of possible differential propagations in DDTs into account or not | choosing reduction methods |
    | Options | | | "AS"; "DC" | "1" : T1; "2" : T2; "3" : T3; "4" : T4; "5" : T5; "6" : T6; "7" : T7; "8" : T8 |  
    
     *E.g.*,  `$ ./EasyDC Present 4,15,3,8,13,10,12,0,11,5,7,14,2,6,1,9 AS 1`
  - set paramters via *[parameters.txt](https://github.com/GOODMORNING616/EasyDC/blob/main/parameters.txt)* file.
    *E.g.*, *[parametersSboxDemo.txt](https://github.com/GOODMORNING616/EasyDC/blob/main/parametersSboxDemo.txt)*  
    
    ```
    $ cat parametersSboxDemo.txt  
    Present  
    4,15,3,8,13,10,12,0,11,5,7,14,2,6,1,9  
    AS  
    1
    ``` 

### 2. Security analysis of block ciphers against differential cryptanalysis
  - comand : "./EasyDC argv[1]  argv[2]  argv[3]  argv[4]  argv[5]  argv[6]  argv[7]  argv[8]  argv[9]  argv[10]  argv[11]  argv[12]  argv[13] " : 

    |  | argv[1] | argv[2] | argv[3] | argv[4] | argv[5] | argv[6]/argv[8]/argv[10]/argv[12] | argv[7]/argv[9]/argv[11]/argv[13] |
    | :-----: | :-----: | :----: | :----: | :----: | :----: | :----: | :----: |
    | Remark | num of parameters | **EasyDC** program file path | modeling approach | mode of modelings for S-boxes | choosing reduction methods for S-boxes | 
    | Options | | | "w" : word-wise approach; "b" : bit-wise approach; "d" : extended bit-wise approach | "AS"; "DC" | "1" : T1; "2" : T2; "3" : T3; "4" : T4; "5" : T5; "6" : T6; "7" : T7; "8" : T8 | startRound or allRounds or timer(second) or threadsNum | startRound or allRounds or timer(second) or threadsNum |
    
    *E.g.*,  `$ ./EasyDC 7 ../benchmarks/BlockCipher/PRESENT.cl b AS 1 allRounds 5`
    
  - set paramters via *[parameters.txt](https://github.com/GOODMORNING616/EasyDC/blob/main/parameters.txt)* file.
    *E.g.*, *[parametersMILPDemo.txt](https://github.com/GOODMORNING616/EasyDC/blob/main/parametersMILPDemo.txt)*  
    
    ```
    $ cat parametersMILPDemo.txt  
    7
    ../benchmarks/BlockCipher/PRESENT.cl
    b
    AS
    1
    allRounds
    5
    ``` 

## Advanced Usage

## Example

## Citations


    
