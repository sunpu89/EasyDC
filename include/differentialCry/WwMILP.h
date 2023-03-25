//
// Created by Septi on 2/4/2023.
//

#ifndef EASYBC_WWMILP_H
#define EASYBC_WWMILP_H

#include "ProcedureH.h"
#include <utility>
#include <utilities.h>
#include "Transformer.h"
#include "SboxModel.h"
#include "gurobi_c++.h"
#include "Reduction.h"
#include "BranchNum.h"
#include "MILPcons.h"

/*
 * Word-wise MILP used to calculate the minimal number of active S-boxes
 * */

class WwMILP {

private:
    std::string cipherName;
    std::vector<ProcedureHPtr> procedureHs;

    int blockSize = 0;
    int wordSize;
    std::map<std::string, std::vector<int>> Box;
    std::map<std::string, int> branchNumMin;
    std::map<std::string, int> branchNumMax;

    std::map<std::string, int> tanNameMxIndex;
    std::map<std::string, int> rtnMxIndex;
    std::vector<int> rtnIdxSave;
    std::vector<int> inputIdxSave;    // index of initial input
    bool sboxFindFlag = false;
    std::vector<int> sboxIndexSave;     // index of variable of S-boxes used for objective function
    int dCounter = 1;
    int ACounter = 1;

    std::string roundID;
    int allSearchRounds = 36;
    int startRound = 1;
    int currMaxSearchRound;
    std::string moPath = std::string(DPATH) + "differential/WordWiseMILP/models/";
    std::string moResults = std::string(DPATH) + "differential/WordWiseMILP/results/";
    std::string modelPath;
    std::string resultsPath;
    std::string finalResultsPath;

    vector<int> finalResults;
    vector<double> finalClockTime;
    vector<double> finalTimeTime;

    // set the IL constraints of XOR and linear transformation
    int xorConsSelect = 1; // 1-2
    int linearConsSelect = 1; // 1-2

    std::string compares;

    int gurobiTimer = 3600 * 12;
    int gurobiThreads = 16;

public:

    WwMILP(const vector<ProcedureHPtr>& procedureHs);

    void MILP();

    void builder();

    void roundFuncMILPbuilder(const ProcedureHPtr& procedureH);

    void sboxFuncMILPbuilder(const ProcedureHPtr& procedureH);

    void XORGenModel(const ThreeAddressNodePtr& left, const ThreeAddressNodePtr& right, const ThreeAddressNodePtr& result);

    void ANDandORGenModel(const ThreeAddressNodePtr& left, const ThreeAddressNodePtr& right, const ThreeAddressNodePtr& result);

    void idxPboxPermute(const ThreeAddressNodePtr& pbox, const ThreeAddressNodePtr& input, const std::vector<ThreeAddressNodePtr>& output);

    void linearPermuteGenModel(const ThreeAddressNodePtr& pbox, const ThreeAddressNodePtr& input, const std::vector<ThreeAddressNodePtr>& output);

    void AddGenModel(std::vector<ThreeAddressNodePtr>& input1, std::vector<ThreeAddressNodePtr>& input2, const std::vector<ThreeAddressNodePtr>& output);

    void solver();

    bool isConstant(ThreeAddressNodePtr threeAddressNodePtr);

    void setStartRound(int start) { this->startRound = start; }
    void setAllRound(int allRnd) { this->allSearchRounds = allRnd; }
    void setThreadNum(int num) { this->gurobiThreads = num; }
    void setTimer(int timer) { this->gurobiTimer = timer; }
    void setConsSelecter(int xors, int linears) {
        xorConsSelect = xors;
        linearConsSelect = linears;
    }
};


#endif //EASYBC_WWMILP_H
