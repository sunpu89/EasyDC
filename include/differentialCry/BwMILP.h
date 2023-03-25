//
// Created by Septi on 2/7/2023.
//

#ifndef EASYBC_BWMILP_H
#define EASYBC_BWMILP_H


#include "ProcedureH.h"
#include <utility>
#include <cmath>
#include <util/utilities.h>
#include "Transformer.h"
#include "Interpreter.h"
#include "Reduction.h"
#include "SyntaxGuided.h"
#include "BranchNum.h"
#include "MILPcons.h"
#include "SboxModel.h"


class BwMILP {

private:
    std::string cipherName;
    vector<ProcedureHPtr> procedureHs;

    int blockSize = 0;
    std::map<std::string, std::vector<int>> Box;
    std::map<std::string, std::vector<std::vector<int>>> sboxIneqs;
    std::map<std::string, bool> sboxIfInjective;
    std::map<std::string, int> branchNumMin;
    std::map<std::string, int> branchNumMax;

    map<std::string, int> tanNameMxIndex;
    map<std::string, int> rtnMxIndex;
    std::vector<int> rtnIdxSave;

    int xCounter = 1;
    int dCounter = 1;
    int ACounter = 1;
    int PCounter = 1;
    int fCounter = 1; // 溢出位结果标识
    int yCounter = 1; // new XOR model2 label

    std::string mode; // AS or DC
    int redMode = 1;
    int allSearchRounds = 5; // Number of all search roundsm
    int startRound = 1;
    int currMaxSearchRound;
    std::string moPath = std::string(DPATH) + "differential/BitWiseMILP/models/";
    std::string moResults = std::string(DPATH) + "differential/BitWiseMILP/results/";
    std::string dpath = std::string(DPATH) + "differential/BitWiseMILP/dc/";
    std::string modelPath;
    std::string resultsPath;
    std::string finalResultsPath;
    std::string dcPath; // save differential characteristics
    // extract differential characteristic
    std::vector<std::vector<int>> differentialCharacteristic;

    std::string roundID;

    // the weight of each extended bit of DC mode
    // used to decide the objective function
    // Here we consider the extension bits to be the same in the case of multiple S-boxes
    std::vector<int> extWeighted = {1};
    bool ILPFlag = true;

    // used to save the input and output size of S-box
    std::map<std::string, int> sboxInputSize;
    std::map<std::string, int> sboxOutputSize;

    // for arx
    std::vector<std::vector<int>> ARXineqs;
    bool arxFlag = false;
    std::vector<int> ARXlinearRem;

    vector<int> finalResults;
    vector<double> finalClockTime;
    vector<double> finalTimeTime;

    int xorConsSelect = 2; // 1-3
    int sboxConsSelect = 1; // 1-3
    int matrixConsSelect = 2; // 1-3

    std::string compares;

    int gurobiTimer = 3600 * 12;
    int gurobiThreads = 16;

public:
    BwMILP(vector<ProcedureHPtr> procedureHs, std::string mode, int red_mode);

    void MILP();

    void builder();

    void roundFBuilder(const ProcedureHPtr& procedureH);

    // gen constraints for various ops

    void XORGenModel(const ThreeAddressNodePtr& left, const ThreeAddressNodePtr& right, const ThreeAddressNodePtr& result);

    void ANDandORGenModel(const ThreeAddressNodePtr& left, const ThreeAddressNodePtr& right, const ThreeAddressNodePtr& result);

    void UintsSboxGenModel(const ThreeAddressNodePtr& sbox, const ThreeAddressNodePtr& input, const ThreeAddressNodePtr& output);

    void Uint1SboxGenModel(const ThreeAddressNodePtr& sbox, const ThreeAddressNodePtr& input, const std::vector<ThreeAddressNodePtr>& output);

    // permute the indexes of index permutation
    void Uint1IdxPboxPermute(const ThreeAddressNodePtr& pbox, const ThreeAddressNodePtr& input, const std::vector<ThreeAddressNodePtr>& output);

    // matrix-vector product
    void MatrixVectorGenModel(const ThreeAddressNodePtr& pboxm, const ThreeAddressNodePtr& input, const std::vector<ThreeAddressNodePtr>& output);
    void multiXORGenModel12(std::vector<ThreeAddressNodePtr> input, ThreeAddressNodePtr result);
    void multiXORGenModel3(std::vector<ThreeAddressNodePtr> input, ThreeAddressNodePtr result);

    // for ARX add
    void Uint1AddMinusGenModel(std::vector<ThreeAddressNodePtr>& input1, std::vector<ThreeAddressNodePtr>& input2,
                               const std::vector<ThreeAddressNodePtr>& output, int AddOrMinus);

    void solver();

    // used to set some parameters
    void setStartRound(int start) { this->startRound = start; }
    void setAllRound(int allRnd) { this->allSearchRounds = allRnd; }
    void setThreadNum(int num) { this->gurobiThreads = num; }
    void setTimer(int timer) { this->gurobiTimer = timer; }
    void setConsSelecter(int xorC, int sboxC, int matrixC) {
        this->xorConsSelect = xorC;
        this->sboxConsSelect = sboxC;
        this->matrixConsSelect = matrixC;
    };


    bool isConstant(ThreeAddressNodePtr threeAddressNodePtr) {
        ThreeAddressNodePtr left = threeAddressNodePtr->getLhs();
        ThreeAddressNodePtr right = threeAddressNodePtr->getRhs();

        if (threeAddressNodePtr->getNodeType() == CONSTANT)
            return true;
        else if (threeAddressNodePtr->getNodeName() == this->roundID)
            return true;
        else if (left == nullptr and right == nullptr)
            return false;
        else if (left != nullptr and right != nullptr) {
            if (left->getNodeType() == CONSTANT and right->getNodeType() == CONSTANT)
                return true;
            else if (left->getNodeType() == CONSTANT and right->getNodeName() == roundID)
                return true;
            else if (right->getNodeType() == CONSTANT and left->getNodeName() == roundID)
                return true;
            else
                return false;
        } else if (left != nullptr and right == nullptr) {
            if (left->getNodeType() == CONSTANT)
                return true;
            if (left->getNodeName() == this->roundID)
                return true;
            else if (isConstant(left))
                return true;
            else
                return false;
        } else if (left == nullptr and right != nullptr) {
            if (right->getNodeType() == CONSTANT)
                return true;
            if (right->getNodeName() == this->roundID)
                return true;
            else if (isConstant(right))
                return true;
            else
                return false;
        } else
            assert(false);
    };

    // extract index form the ThreeAddressNode instance whose op is touint or boxindex
    std::vector<int> extIdxFromTOUINTorBOXINDEX(const ThreeAddressNodePtr& input) {
        std::vector<int> extIdx;
        if (input->getOp() == ASTNode::TOUINT) {
            ThreeAddressNodePtr left = input->getLhs();
            while (left->getOp() == ASTNode::BOXINDEX) {
                // Before the specific treatment, we need to add a judgment to  whether the input three-address instances
                // have been stored in the map of tanNameMxIndex.
                // If there is, there is no need to repeat the processing and directly end the operation of the function
                if (tanNameMxIndex.count(left->getLhs()->getNodeName()) != 0) {
                    extIdx.push_back(tanNameMxIndex.find(left->getLhs()->getNodeName())->second);
                    left = left->getRhs();
                }
                    // If it is not the first round, i.e. rtnRecorder is not empty,
                    // the first element of rtnRecorder is taken each time as the ordinal number of x
                else if (!rtnIdxSave.empty()) {
                    tanNameMxIndex[left->getLhs()->getNodeName()] = rtnIdxSave.front();
                    extIdx.push_back(rtnIdxSave.front());
                    rtnIdxSave.erase(rtnIdxSave.cbegin());
                    left = left->getRhs();
                } else {
                    tanNameMxIndex[left->getLhs()->getNodeName()] = xCounter;
                    extIdx.push_back(xCounter);
                    xCounter++;
                    left = left->getRhs();
                }
            }
            if (!rtnIdxSave.empty()) {
                tanNameMxIndex[left->getLhs()->getNodeName()] = rtnIdxSave.front();
                extIdx.push_back(rtnIdxSave.front());
                rtnIdxSave.erase(rtnIdxSave.cbegin());
            } else if (tanNameMxIndex.count(left->getNodeName()) != 0) {
                extIdx.push_back(tanNameMxIndex.find(left->getNodeName())->second);
            } else {
                tanNameMxIndex[left->getLhs()->getNodeName()] = xCounter;
                extIdx.push_back(xCounter);
                xCounter++;
            }
        } else if (input->getOp() == ASTNode::BOXINDEX) {
            ThreeAddressNodePtr left = input;
            while (left->getOp() == ASTNode::BOXINDEX) {
                if (tanNameMxIndex.count(left->getLhs()->getNodeName()) != 0) {
                    extIdx.push_back(tanNameMxIndex.find(left->getLhs()->getNodeName())->second);
                    left = left->getRhs();
                } else if (!rtnIdxSave.empty()) {
                    tanNameMxIndex[left->getLhs()->getNodeName()] = rtnIdxSave.front();
                    extIdx.push_back(rtnIdxSave.front());
                    rtnIdxSave.erase(rtnIdxSave.cbegin());
                    left = left->getRhs();
                } else {
                    tanNameMxIndex[left->getLhs()->getNodeName()] = xCounter;
                    extIdx.push_back(xCounter);
                    xCounter++;
                    left = left->getRhs();
                }
            }
            if (!rtnIdxSave.empty()) {
                tanNameMxIndex[left->getNodeName()] = rtnIdxSave.front();
                extIdx.push_back(rtnIdxSave.front());
                rtnIdxSave.erase(rtnIdxSave.cbegin());
            } else if (tanNameMxIndex.count(left->getNodeName()) != 0) {
                extIdx.push_back(tanNameMxIndex.find(left->getNodeName())->second);
            } else {
                tanNameMxIndex[left->getNodeName()] = xCounter;
                extIdx.push_back(xCounter);
                xCounter++;
            }
        } else assert(false);
        return extIdx;
    }

    void sboxSizeGet(std::string name, std::vector<int> sbox) {
        sboxInputSize[name] = int(log2(sbox.size()));
        map<int, int> sboxMap;
        for (int & i : sbox) {
            sboxMap[i]++;
        }
        sboxOutputSize[name] = int(log2(sboxMap.size()));
    }

    static std::vector<int> shiftVec(std::vector<int> input, int shiftNum) {
        std::vector<int> rtn;
        for (int i = shiftNum; i < input.size(); ++i) {
            rtn.push_back(input[i]);
        }
        for (int i = rtn.size(); i < input.size(); ++i) {
            rtn.push_back(0);
        }
        return rtn;
    }

    // return false -> noninjective
    // return true -> injective
    static bool sboxInjectiveCheck(std::vector<int> sbox) {
        set<int> arr;
        pair<set<int>::iterator,bool> pr;
        for(auto it=sbox.begin();it<sbox.end();it++){
            pr = arr.insert(*it);
            if(!pr.second){
                return false;
            }
        }
        return true;
    }

    static int transNodeTypeSize(NodeType nodeType) {
        if (nodeType == UINT) {
            return 0;
        }
        else if (nodeType == UINT1) {
            return 1;
        }
        else if (nodeType == UINT4) {
            return 4;
        }
        else if (nodeType == UINT6) {
            return 6;
        }
        else if (nodeType == UINT8) {
            return 8;
        }
        else if (nodeType == UINT10) {
            return 10;
        }
        else if (nodeType == UINT16) {
            return 16;
        }
        else if (nodeType == UINT32) {
            return 32;
        }
        else if (nodeType == UINT64) {
            return 64;
        }
        else if (nodeType == UINT128) {
            return 128;
        }
        else if (nodeType == UINT256) {
            return 256;
        }
        else if (nodeType == UINT512) {
            return 512;
        }
        return 0;
    }
};


#endif //EASYBC_BWMILP_H
