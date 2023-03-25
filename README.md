# EasyDC: A Domain-Specific Language for Security Analysis of Block Ciphers against Differential Cryptanalysis

This repository is for parsing the input EasyDC programs of block ciphers and then
performing the security analysis of block ciphers against differential cryptanalysis
by solving MILP instances generated from the preprocessed programs of inputs.
It also support the modelings of S-boxes separately, which generate IL constraints
of DDTs of the given S-boxes.

Preparatory environment:
1. gurobi
2. z3
3. flex
4. bison
5. jsoncpp

Usage:
1. Modeling possible differential propagations in DDTs of S-boxes

2. Security analysis of block ciphers against differential cryptanalysis
