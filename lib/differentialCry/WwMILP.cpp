//
// Created by Septi on 2/6/2023.
//
#include "WwMILP.h"

extern std::map<std::string, std::vector<int>> allBox;
extern map<std::string, std::vector<int>> pboxM;
extern std::map<std::string, int> pboxMSize;
extern map<std::string, std::vector<int>> Ffm;
extern std::string cipherName;


WwMILP::WwMILP(const vector<ProcedureHPtr> &procedureHs) : procedureHs(procedureHs)  {
    this->Box = allBox;
    this->cipherName = ::cipherName;
    // obtain branch number of all boxes
    auto iterator = Box.begin();
    while (iterator != Box.end()) {
        branchNumMin[iterator->first] = Red::branch_num_of_sbox(iterator->second);
        iterator++;
    }
    auto iterM = pboxM.begin();
    while (iterM != pboxM.end()) {
        BranchN branchN(iterM->second, Ffm[iterM->first], "w");
        std::vector<int> branches = branchN.getBranchNum();
        branchNumMin[iterM->first] = branches[0];
        branchNumMax[iterM->first] = branches[1];
        iterM++;
    }
}

void WwMILP::MILP() {
    compares = "xor" + std::to_string(xorConsSelect) + "_linear" + std::to_string(linearConsSelect) + "/";

    // initial directories
    this->modelPath = this->moPath + this->cipherName + "/" + compares;
    this->resultsPath = this->moResults + this->cipherName + "/" + compares;
    system(("mkdir -p " + modelPath).c_str());
    system(("mkdir -p " + resultsPath).c_str());
    this->finalResultsPath = this->resultsPath + this->cipherName + "_FinalResults.txt";
    system(("touch " + this->finalResultsPath).c_str());
    for (int i = this->startRound; i <= allSearchRounds; ++i) {
        this->currMaxSearchRound = i;
        std::cout << "\n **************** CURRENT MAX ROUOND : " << this->currMaxSearchRound << "  **************** \n" << std::endl;
        // initial path
        this->modelPath = this->moPath + this->cipherName + "/" + compares + std::to_string(this->currMaxSearchRound) + "_round_model.lp";
        this->resultsPath = this->moResults + this->cipherName + "/" + compares + std::to_string(this->currMaxSearchRound) + "_round_results.txt";
        system(("touch " + this->modelPath).c_str());
        system(("touch " + this->resultsPath).c_str());
        // build models and solve them
        builder();
        solver();
        // initial for next round
        this->tanNameMxIndex.clear();
        this->rtnMxIndex.clear();
        this->rtnIdxSave.clear();
        this->inputIdxSave.clear();
        this->dCounter = 1;
        this->ACounter = 1;
        sboxFindFlag = false;
        sboxIndexSave.clear();
        // save the current final results
        std::ofstream fResults(this->finalResultsPath, std::ios::trunc);
        if (!fResults){
            std::cout << "Wrong file path ! " << std::endl;
        } else {
            fResults << " ***************** FINAL RESULTS ***************** \n\n";
            for (int j = 0; j < finalResults.size(); ++j) {
                fResults << "Results Of " << j + 1 << " Rounds : \n";
                fResults << "\tObj : " << finalResults[j] << "\n";
                fResults << "\tclockTime : " << finalClockTime[j] << "\n";
                fResults << "\ttimeTime : " << finalTimeTime[j] << "\n\n";
            }
            fResults << " ***************** FINAL RESULTS ***************** \n";
            fResults.close();
        }
    }
    // print final results
    std::cout << "\n ***************** FINAL RESULTS ***************** " << std::endl;
    for (int i = 0; i < finalResults.size(); ++i) {
        std::cout << "Results Of " << i + 1 << " Rounds : " << std::endl;
        std::cout << "\tObj : " << finalResults[i] << std::endl;
        std::cout << "\tclockTime : " << finalClockTime[i] << std::endl;
        std::cout << "\ttimeTime : " << finalTimeTime[i] << std::endl << std::endl;
    }
    std::cout << " ***************** FINAL RESULTS ***************** \n" << std::endl;
}

void WwMILP::builder() {
    std::ofstream scons(this->modelPath, std::ios::trunc);
    if (!scons){
        std::cout << "Wrong file path ! " << std::endl;
    } else {
        scons << "Subject To\n";
        scons.close();
    }
    int roundCounter = currMaxSearchRound;
    for (const auto& proc : procedureHs) {
        if (proc->getName() == "main") {
            std::string roundFuncId;
            bool newRoundFlag = false;
            int tempSizeCounter = 0;
            int roundFlag = false, modelCheckFlag = true;
            for (const auto& ele : proc->getBlock()) {
                if (ele->getLhs()->getNodeType() == CONSTANT) {
                    newRoundFlag = true;
                    continue;
                }
                if (newRoundFlag and roundCounter > 0) {
                    if (ele->getNodeName() == "plaintext_push")
                        tempSizeCounter++;
                    if (ele->getOp() == ASTNode::CALL) {
                        this->blockSize = tempSizeCounter;
                        tempSizeCounter = 0;
                        roundFuncId = ele->getLhs()->getNodeName().substr(0, ele->getLhs()->getNodeName().find("@"));
                        for (const auto& tproc : procedureHs) {
                            if (tproc->getName() == roundFuncId) {
                                roundFuncMILPbuilder(tproc);
                                roundFlag = true;
                                break;
                            }
                        }
                        newRoundFlag = false;
                        roundCounter--;
                    }
                }
                if (roundFlag and modelCheckFlag) {
                    std::ifstream file;
                    file.open(modelPath);
                    std::string model, line;
                    while (getline(file, line)) model = model + line + "\n";
                    file.close();
                    if (model == "Subject To\n") {
                        // initial for next round;
                        this->tanNameMxIndex.clear();
                        this->rtnMxIndex.clear();
                        this->rtnIdxSave.clear();
                        this->inputIdxSave.clear();
                        this->dCounter = 1;
                        this->ACounter = 1;
                        roundFlag = false;
                    } else
                        modelCheckFlag = false;
                }
            }
            break;
        }
    }
    std::ifstream file;
    file.open(modelPath);
    std::string model,line;
    while (getline(file, line))
        model = model + line + "\n";
    file.close();

    if (model != "Subject To\n") {
        std::ofstream target(this->modelPath, std::ios::trunc);
        if (!target){
            std::cout << "Wrong file path ! " << std::endl;
        } else {
            target << "Minimize\n";
            for (int i = 0; i < sboxIndexSave.size(); ++i) {
                target << "A" << sboxIndexSave[i];
                if (i != sboxIndexSave.size() - 1) {
                    target << " + ";
                }
            }
            target << "\n";
            target << model;
            for (int i = 0; i < blockSize; ++i) {
                if (i != blockSize - 1)
                    target << "A" << inputIdxSave[i] << " + ";
                else
                    target << "A" << inputIdxSave[i];
            }
            target << " >= 1\n";
            target.close();
        }
    }
    std::ofstream binary(this->modelPath, std::ios::app);
    if (!binary){
        std::cout << "Wrong file path ! " << std::endl;
    } else {
        binary << "Binary\n";
        for (int i = 1; i < dCounter; ++i)
            binary << "d" << i << "\n";
        for (int i = 1; i < ACounter; ++i)
            binary << "A" << i << "\n";
        binary << "End";
        binary.close();
    }
}

void WwMILP::roundFuncMILPbuilder(const ProcedureHPtr &procedureH) {
    std::string roundId, keyId, plaintextId;
    roundId = procedureH->getParameters().at(0).at(0)->getNodeName();
    this->roundID = roundId;
    keyId = procedureH->getParameters().at(1).at(0)->getNodeName().substr(0, procedureH->getParameters().at(1).at(0)->getNodeName().find("0"));
    plaintextId = procedureH->getParameters().at(2).at(0)->getNodeName().substr(0, procedureH->getParameters().at(2).at(0)->getNodeName().find("0"));

    this->wordSize = Transformer::getNodeTypeSize(procedureH->getReturns().at(0)->getNodeType());

    if (!rtnIdxSave.empty()) {
        for (int i = 0; i < rtnIdxSave.size(); ++i)
            tanNameMxIndex[procedureH->getParameters().at(2).at(i)->getNodeName()] = rtnIdxSave[i];
        rtnIdxSave.clear();
        rtnMxIndex.clear();
    } else {
        for (const auto & i : procedureH->getParameters().at(2)) {
            tanNameMxIndex[i->getNodeName()] = ACounter;
            if (inputIdxSave.size() < blockSize)
                inputIdxSave.push_back(ACounter);
            ACounter++;
        }
    }

    bool functionCallFlag;
    for (int i = 0; i < procedureH->getBlock().size(); ++i) {
        functionCallFlag = false;
        ThreeAddressNodePtr ele = procedureH->getBlock().at(i);

        if (ele->getOp() == ASTNode::XOR) {
            if (ele->getLhs()->getNodeName().find(keyId) != std::string::npos or ele->getRhs()->getNodeName().find(keyId) != std::string::npos) {}
            else if (this->isConstant(ele->getLhs()) or this->isConstant(ele->getRhs())) {}
            else {
                functionCallFlag = true;
                XORGenModel(ele->getLhs(), ele->getRhs(), ele);
            }
        } else if (ele->getOp() == ASTNode::AND or ele->getOp() == ASTNode::OR) {
            functionCallFlag = true;
            ANDandORGenModel(ele->getLhs(), ele->getRhs(), ele);
        } else if (ele->getOp() == ASTNode::BOXOP) {
            if (ele->getLhs()->getNodeName().substr(0, 4) == "sbox") {
                sboxFindFlag = true;
            } else if (ele->getLhs()->getNodeName().substr(0, 4) == "pbox") {
                functionCallFlag = true;
                if (ele->getNodeType() != NodeType::UINT1 and Transformer::getNodeTypeSize(ele->getNodeType()) != this->wordSize)
                    assert(false);
                else {
                    std::vector<ThreeAddressNodePtr> output;
                    std::string pboxName = ele->getLhs()->getLhs()->getNodeName();
                    while (true) {
                        output.push_back(ele);
                        i++;
                        if (i == procedureH->getBlock().size()) {
                            i--;
                            break;
                        }
                        ele = procedureH->getBlock().at(i);
                        if (ele != nullptr and ele->getOp() == ASTNode::BOXOP and ele->getLhs()->getNodeName() == pboxName) {
                            i--;
                            continue;
                        } else
                            break;
                    }
                    idxPboxPermute(ele->getLhs(), ele->getRhs(), output);
                }
            }
        } else if (ele->getOp() == ASTNode::SYMBOLINDEX) {
            ThreeAddressNodePtr left = ele->getLhs();
            if (left->getOp() == ASTNode::BOXOP) {
                if (left->getLhs()->getNodeName().substr(0, 4) == "sbox") {
                    sboxFindFlag = true;
                } else if (left->getLhs()->getNodeName().substr(0, 4) == "pbox") {
                    functionCallFlag = true;
                    if (ele->getNodeType() != NodeType::UINT1 and Transformer::getNodeTypeSize(ele->getNodeType()) != this->wordSize)
                        assert(false);
                    else {
                        std::vector<ThreeAddressNodePtr> output;
                        std::string pboxName = ele->getLhs()->getLhs()->getNodeName();
                        while (true) {
                            output.push_back(ele);
                            i++;
                            if (i == procedureH->getBlock().size()) {
                                i--;
                                break;
                            } else {
                                ele = procedureH->getBlock().at(i);
                                left = ele->getLhs();
                                if (left != nullptr and left->getOp() == ASTNode::BOXOP and left->getLhs()->getNodeName() == pboxName) {
                                    continue;
                                } else {
                                    i--;
                                    left = procedureH->getBlock().at(i)->getLhs();
                                    break;
                                }
                            }
                        }
                        idxPboxPermute(left->getLhs(), left->getRhs(), output);
                    }
                } else
                    assert(false);
            } else if (left->getOp() == ASTNode::FFTIMES) {
                if (left->getLhs()->getNodeName().substr(0, 5) == "pboxm") {
                    functionCallFlag = true;
                    std::vector<ThreeAddressNodePtr> output;
                    int pboxSize = pboxM[left->getLhs()->getNodeName()].size();
                    int outputNum = (int)sqrt(pboxSize);
                    while (outputNum > 0) {
                        output.push_back(ele);
                        i++;
                        if (i == procedureH->getBlock().size()) {
                            i--;
                            break;
                        } else {
                            ele = procedureH->getBlock().at(i);
                            left = ele->getLhs();
                            if (left != nullptr and left->getOp() == ASTNode::FFTIMES and
                                left->getLhs()->getNodeName().substr(0, 5) == "pboxm") {
                                outputNum--;
                                continue;
                            } else {
                                i--;
                                left = procedureH->getBlock().at(i)->getLhs();
                                break;
                            }
                        }
                    }
                    if (outputNum == 0)
                        i--;
                    left = procedureH->getBlock().at(i)->getLhs();
                    linearPermuteGenModel(left->getLhs(), left->getRhs(), output);
                } else
                    assert(false);
            } else if (left->getOp() == ASTNode::NULLOP) {}
            else if (left->getOp() == ASTNode::SYMBOLINDEX) {}
            else
                assert(false);
        } else if (ele->getOp() == ASTNode::TOUINT) {}
        else if (ele->getOp() == ASTNode::INDEX) {}
        else if (ele->getOp() == ASTNode::ADD or ele->getOp() == ASTNode::MINUS) {
            functionCallFlag = true;
            std::string outputName = ele->getNodeName().substr(0, ele->getNodeName().find_last_of("_"));
            std::vector<ThreeAddressNodePtr> input1, input2, output;
            while (true) {
                input1.push_back(ele->getLhs());
                input2.push_back(ele->getRhs());
                output.push_back(ele);
                i++;
                if (i == procedureH->getBlock().size()) {
                    i--;
                    break;
                }
                ele = procedureH->getBlock().at(i);
                std::string  aaa = ele->getNodeName().substr(0, outputName.size());

                if (ele != nullptr and ele->getOp() == ASTNode::ADD and ele->getNodeName().substr(0, outputName.size()) == outputName)
                    continue;
                else {
                    i--;
                    break;
                }
            }
            AddGenModel(input1, input2, output);
        }
        else assert(false);

        if (!functionCallFlag) {
            for (const auto& pair : tanNameMxIndex) {
                if (ele->getLhs() != nullptr)
                    if (ele->getLhs()->getNodeName() == pair.first)
                        tanNameMxIndex[ele->getNodeName()] = pair.second;
                if (ele->getRhs() != nullptr)
                    if (ele->getRhs()->getNodeName() == pair.first)
                        tanNameMxIndex[ele->getNodeName()] = pair.second;
            }
        } else
            functionCallFlag = false;

        if (sboxFindFlag) {
            sboxFindFlag = false;
            for (const auto& name : tanNameMxIndex) {
                if (name.first == ele->getNodeName())
                    sboxIndexSave.push_back(name.second);
            }
        }
    }

    for (const auto& rtn : procedureH->getReturns()) {
        for (const auto& pair : tanNameMxIndex) {
            if (rtn->getNodeName() == pair.first) {
                rtnMxIndex[pair.first] = pair.second;
                rtnIdxSave.push_back(pair.second);
            }
        }
    }
}

void WwMILP::XORGenModel(const ThreeAddressNodePtr &left, const ThreeAddressNodePtr &right,
                         const ThreeAddressNodePtr &result) {
    int inputIdx1 = 0, inputIdx2 = 0;
    for (const auto& pair : tanNameMxIndex) {
        if (pair.first == left->getNodeName())
            inputIdx1 = pair.second;
        if (pair.first == right->getNodeName())
            inputIdx2 = pair.second;
    }
    std::vector<int> inputIdx = {inputIdx1, inputIdx2};
    for (auto item : inputIdx) {
        if (item == 0) {
            if (!this->rtnIdxSave.empty()) {
                item = this->rtnIdxSave.front();
                this->rtnIdxSave.erase(this->rtnIdxSave.cbegin());
                this->tanNameMxIndex[left->getNodeName()] = item;
            } else {
                item = this->ACounter;
                this->tanNameMxIndex[left->getNodeName()] = item;
                this->ACounter++;
            }
        }
    }
    int outputIdx = ACounter;
    tanNameMxIndex[result->getNodeName()] = outputIdx;
    ACounter++;
    switch (xorConsSelect) {
        case 1:
            MILPcons::wXorC1(this->modelPath, inputIdx1, inputIdx2, outputIdx, dCounter);
            break;
        case 2:
            MILPcons::wXorC2(this->modelPath, inputIdx1, inputIdx2, outputIdx);
            break;
        default:
            MILPcons::wXorC1(this->modelPath, inputIdx1, inputIdx2, outputIdx, dCounter);
    }
}

void WwMILP::ANDandORGenModel(const ThreeAddressNodePtr &left, const ThreeAddressNodePtr &right,
                         const ThreeAddressNodePtr &result) {
    int inputIdx1 = 0, inputIdx2 = 0;
    for (const auto& pair : tanNameMxIndex) {
        if (pair.first == left->getNodeName())
            inputIdx1 = pair.second;
        if (pair.first == right->getNodeName())
            inputIdx2 = pair.second;
    }
    std::vector<int> inputIdx = {inputIdx1, inputIdx2};
    for (auto item : inputIdx) {
        if (item == 0) {
            if (!this->rtnIdxSave.empty()) {
                item = this->rtnIdxSave.front();
                this->rtnIdxSave.erase(this->rtnIdxSave.cbegin());
                this->tanNameMxIndex[left->getNodeName()] = item;
            } else {
                item = this->ACounter;
                this->tanNameMxIndex[left->getNodeName()] = item;
                this->ACounter++;
            }
        }
    }
    int outputIdx = ACounter;
    tanNameMxIndex[result->getNodeName()] = outputIdx;
    ACounter++;
    MILPcons::wAndOrC(this->modelPath, inputIdx1, inputIdx2, outputIdx);
}

void WwMILP::idxPboxPermute(const ThreeAddressNodePtr &pbox, const ThreeAddressNodePtr &input,
                            const vector<ThreeAddressNodePtr> &output) {
    std::vector<int> pboxValue = Box[pbox->getNodeName()];
    std::vector<int> inputIdx;
    std::vector<ThreeAddressNodePtr> inputTAN;
    if (input->getOp() == ASTNode::BOXINDEX) {
        ThreeAddressNodePtr left = input;
        while (left->getOp() == ASTNode::BOXINDEX) {
            inputTAN.push_back(left->getLhs());
            left = left->getRhs();
        }
        inputTAN.push_back(left);
    } else
        assert(false);

    for (auto & i : inputTAN) {
        bool iFlag = false;
        for (const auto& pair : tanNameMxIndex) {
            if (pair.first == i->getNodeName()) {
                inputIdx.push_back(pair.second);
                iFlag = true;
            }
        }
        if (!iFlag) {
            inputIdx.push_back(ACounter);
            ACounter++;
        }
    }

    std::vector<int> outputIdx;
    for (int i = 0; i < output.size(); ++i)
        outputIdx.push_back(0);
    for (int i = 0; i < output.size(); ++i)
        //outputIdx[pboxValue.size() - 1 - pboxValue[pboxValue.size() - 1 - i]] = inputIdx[i];
        //outputIdx[pboxValue[i]] = inputIdx[i];
        outputIdx[i] = inputIdx[pboxValue[i]];
    for (int i = 0; i < outputIdx.size(); ++i)
        tanNameMxIndex[output.at(i)->getNodeName()] = outputIdx[i];
}

void WwMILP::linearPermuteGenModel(const ThreeAddressNodePtr &pbox, const ThreeAddressNodePtr &input,
                                   const vector<ThreeAddressNodePtr> &output) {
    std::vector<int> inputIdx;
    std::vector<ThreeAddressNodePtr> inputTAN;
    if (input->getOp() == ASTNode::BOXINDEX) {
        ThreeAddressNodePtr left = input;
        while (left->getOp() == ASTNode::BOXINDEX) {
            inputTAN.push_back(left->getLhs());
            left = left->getRhs();
        }
        inputTAN.push_back(left);
    } else
        assert(false);

    for (auto & i : inputTAN) {
        bool iFlag = false;
        for (const auto& pair : tanNameMxIndex) {
            if (pair.first == i->getNodeName()) {
                inputIdx.push_back(pair.second);
                iFlag = true;
            }
        }
        if (!iFlag) {
            inputIdx.push_back(ACounter);
            tanNameMxIndex[i->getNodeName()] = ACounter;
            ACounter++;
        }
    }

    std::vector<int> outputIdx;
    for (const auto& i : output) {
        outputIdx.push_back(ACounter);
        tanNameMxIndex[i->getNodeName()] = ACounter;
        ACounter++;
    }
    switch (linearConsSelect) {
        case 1:
            MILPcons::wLinearC1(this->modelPath, inputIdx, outputIdx, dCounter, branchNumMin[pbox->getNodeName()], branchNumMax[pbox->getNodeName()]);
            break;
        case 2:
            MILPcons::wLinearC2(this->modelPath, inputIdx,  outputIdx, dCounter, branchNumMin[pbox->getNodeName()], branchNumMax[pbox->getNodeName()]);
            break;
        default:
            MILPcons::wLinearC1(this->modelPath, inputIdx, outputIdx, dCounter, branchNumMin[pbox->getNodeName()], branchNumMax[pbox->getNodeName()]);
    }
}

void WwMILP::AddGenModel(vector<ThreeAddressNodePtr> &input1, vector<ThreeAddressNodePtr> &input2,
                         const vector<ThreeAddressNodePtr> &output) {
    std::vector<int> inputIdx1, inputIdx2;
    for (auto in : input1) {
        if (tanNameMxIndex.count(in->getNodeName()) != 0) {
            inputIdx1.push_back(tanNameMxIndex.find(in->getNodeName())->second);
            if (inputIdxSave.size() < blockSize)
                inputIdxSave.push_back(tanNameMxIndex.find(in->getNodeName())->second);
        } else if (!rtnIdxSave.empty()) {
            tanNameMxIndex[in->getNodeName()] = rtnIdxSave.front();
            inputIdx1.push_back(rtnIdxSave.front());
            if (inputIdxSave.size() < blockSize)
                inputIdxSave.push_back(rtnIdxSave.front());
            rtnIdxSave.erase(rtnIdxSave.cbegin());
        } else {
            tanNameMxIndex[in->getNodeName()] = ACounter;
            inputIdx1.push_back(ACounter);
            if (inputIdxSave.size() < blockSize)
                inputIdxSave.push_back(ACounter);
            ACounter++;
        }
    }
    for (auto in : input2) {
        if (tanNameMxIndex.count(in->getNodeName()) != 0) {
            inputIdx2.push_back(tanNameMxIndex.find(in->getNodeName())->second);
            if (inputIdxSave.size() < blockSize)
                inputIdxSave.push_back(tanNameMxIndex.find(in->getNodeName())->second);
        } else if (!rtnIdxSave.empty()) {
            tanNameMxIndex[in->getNodeName()] = rtnIdxSave.front();
            inputIdx2.push_back(rtnIdxSave.front());
            if (inputIdxSave.size() < blockSize)
                inputIdxSave.push_back(rtnIdxSave.front());
            rtnIdxSave.erase(rtnIdxSave.cbegin());
        } else {
            tanNameMxIndex[in->getNodeName()] = ACounter;
            inputIdx2.push_back(ACounter);
            if (inputIdxSave.size() < blockSize)
                inputIdxSave.push_back(ACounter);
            ACounter++;
        }
    }

    std::vector<int> outputIdx;
    for (const auto& ele : output) {
        tanNameMxIndex[ele->getNodeName()] = ACounter;
        outputIdx.push_back(ACounter);
        ACounter++;
    }

    for (int i = 0; i < inputIdx1.size(); ++i) {
        switch (xorConsSelect) {
            case 1:
                MILPcons::wXorC1(this->modelPath, inputIdx1[i], inputIdx2[i], outputIdx[i], dCounter);
                break;
            case 2:
                MILPcons::wXorC2(this->modelPath, inputIdx1[i], inputIdx2[i], outputIdx[i]);
                break;
            default:
                MILPcons::wXorC1(this->modelPath, inputIdx1[i], inputIdx2[i], outputIdx[i], dCounter);
        }
    }
}

void WwMILP::solver() {
    clock_t startTime, endTime;
    startTime = clock();
    time_t star_time = 0, end_time;
    star_time = time(NULL);
    GRBEnv env = GRBEnv(true);
    env.start();

    // 不是比较测试时不设置线程数
    // LBlock比较时没有设置线程数，其他block cipger比较时再设置
    env.set(GRB_IntParam_Threads, this->gurobiThreads);


    GRBModel model = GRBModel(env, modelPath);
    model.set(GRB_IntParam_MIPFocus, 2);
    model.set(GRB_DoubleParam_TimeLimit, this->gurobiTimer);
    model.optimize();
    endTime = clock();
    end_time = time(NULL);

    int optimstatus = model.get(GRB_IntAttr_Status);
    int result = 0;
    double clockTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    double timeTime = difftime( end_time, star_time);
    if (optimstatus == 2)
        result = model.get(GRB_DoubleAttr_ObjVal);

    std::ofstream f(resultsPath, std::ios::trunc);
    f << "solving the MILP " << modelPath << "\n";
    f << "obj is : " << result << "\n";
    f << "clock time : " << clockTime << "s\n";
    f << "time time : " << timeTime << "s\n";

    GRBVar *a = model.getVars();
    int numvars = model.get(GRB_IntAttr_NumVars);
    for (int i = 0; i < numvars; ++i)
        f << a[i].get(GRB_StringAttr_VarName) << " = " << a[i].get(GRB_DoubleAttr_X) << "\n";
    f.close();
    std::cout << "***********************************" << std::endl;
    std::cout << "      Obj is : " << result << std::endl;
    std::cout << "***********************************" << std::endl;
    finalResults.push_back(result);
    finalClockTime.push_back(clockTime);
    finalTimeTime.push_back(timeTime);
}

bool WwMILP::isConstant(ThreeAddressNodePtr threeAddressNodePtr) {
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
}

// 暂时先不处理，等bit-wise这部分搞定以后，再回头处理word-wise的sbox的函数调用
// 这里我要想一下怎么处理sbox的函数，即轮函数中又调用的函数。
void WwMILP::sboxFuncMILPbuilder(const ProcedureHPtr &procedureH) {

    // 这里可能要分两种情况，一种是输入参数是一个变量，另一种是输入参数是一个vector，那么我们就需要分开进行处理：
    // 1. 对于单个变量的类型：
    //      1） 提取id，根据id的操作以及中间变量的声明，结果变量的声明，来提取operator，并生成相应的约束
    //          那么中间变量和结果变量的声明如何提取呢？
    //      2） 函数最终的结果变量的处理即函数的返回值，返回值需要直接映射到模型boolean 变量的index，
    //          那么是返回前映射还是返回后映射呢？
    //      3） 需要再约束生成时，类似于roundFn时的处理，给定一个flag来标记函数调用嘛？
    //      4） 绑定sbox的变量是否应该根据输入的input来确定呢？ 应该是的，active sbox的确定时input difference非0

    // 2. 对于vector类型的变量：
    //    参考roundFn的实现，但是需要考虑以下几点：
    //      1） 中间变量和结果变量的声明是否需要和输入参数保持一致呢？
    //          如果可以不一致并且确实不一致，需要分开单独处理吗？
    //      2） 上述 1 中的 2),3)和4）

    std::string inputId;
    inputId = procedureH->getParameters().at(0).at(0)->getNodeName();

    if (!rtnIdxSave.empty()) {
        for (int i = 0; i < rtnIdxSave.size(); ++i)
            tanNameMxIndex[procedureH->getParameters().at(2).at(i)->getNodeName()] = rtnIdxSave[i];
        rtnIdxSave.clear();
        rtnMxIndex.clear();
    } else {
        for (const auto & i : procedureH->getParameters().at(2)) {
            tanNameMxIndex[i->getNodeName()] = ACounter;
            if (inputIdxSave.size() < blockSize)
                inputIdxSave.push_back(ACounter);
            ACounter++;
        }
    }

    bool functionCallFlag;
    for (int i = 0; i < procedureH->getBlock().size(); ++i) {
        functionCallFlag = false;
        ThreeAddressNodePtr ele = procedureH->getBlock().at(i);

        if (ele->getOp() == ASTNode::XOR) {
            /*if (ele->getLhs()->getNodeName().find(keyId) != std::string::npos or ele->getRhs()->getNodeName().find(keyId) != std::string::npos) {}
            else*/
            if (this->isConstant(ele->getLhs()) or this->isConstant(ele->getRhs())) {}
            else {
                functionCallFlag = true;
                XORGenModel(ele->getLhs(), ele->getRhs(), ele);
            }
        } else if (ele->getOp() == ASTNode::AND or ele->getOp() == ASTNode::OR) {
            functionCallFlag = true;
            ANDandORGenModel(ele->getLhs(), ele->getRhs(), ele);
        } else if (ele->getOp() == ASTNode::BOXOP) {
            if (ele->getLhs()->getNodeName().substr(0, 4) == "sbox") {
                sboxFindFlag = true;
            } else if (ele->getLhs()->getNodeName().substr(0, 4) == "pbox") {
                functionCallFlag = true;
                if (ele->getNodeType() != NodeType::UINT1 and Transformer::getNodeTypeSize(ele->getNodeType()) != this->wordSize)
                    assert(false);
                else {
                    std::vector<ThreeAddressNodePtr> output;
                    std::string pboxName = ele->getLhs()->getLhs()->getNodeName();
                    while (true) {
                        output.push_back(ele);
                        i++;
                        if (i == procedureH->getBlock().size()) {
                            i--;
                            break;
                        }
                        ele = procedureH->getBlock().at(i);
                        if (ele != nullptr and ele->getOp() == ASTNode::BOXOP and ele->getLhs()->getNodeName() == pboxName) {
                            i--;
                            continue;
                        } else
                            break;
                    }
                    idxPboxPermute(ele->getLhs(), ele->getRhs(), output);
                }
            }
        } else if (ele->getOp() == ASTNode::SYMBOLINDEX) {
            ThreeAddressNodePtr left = ele->getLhs();
            if (left->getOp() == ASTNode::BOXOP) {
                if (left->getLhs()->getNodeName().substr(0, 4) == "sbox") {
                    sboxFindFlag = true;
                } else if (left->getLhs()->getNodeName().substr(0, 4) == "pbox") {
                    functionCallFlag = true;
                    if (ele->getNodeType() != NodeType::UINT1 and Transformer::getNodeTypeSize(ele->getNodeType()) != this->wordSize)
                        assert(false);
                    else {
                        std::vector<ThreeAddressNodePtr> output;
                        std::string pboxName = ele->getLhs()->getLhs()->getNodeName();
                        while (true) {
                            output.push_back(ele);
                            i++;
                            if (i == procedureH->getBlock().size()) {
                                i--;
                                break;
                            } else {
                                ele = procedureH->getBlock().at(i);
                                left = ele->getLhs();
                                if (left != nullptr and left->getOp() == ASTNode::BOXOP and left->getLhs()->getNodeName() == pboxName) {
                                    continue;
                                } else {
                                    i--;
                                    left = procedureH->getBlock().at(i)->getLhs();
                                    break;
                                }
                            }
                        }
                        idxPboxPermute(left->getLhs(), left->getRhs(), output);
                    }
                } else
                    assert(false);
            } else if (left->getOp() == ASTNode::FFTIMES) {
                if (left->getLhs()->getNodeName().substr(0, 5) == "pboxm") {
                    functionCallFlag = true;
                    std::vector<ThreeAddressNodePtr> output;
                    int pboxSize = pboxM[left->getLhs()->getNodeName()].size();
                    int outputNum = (int)sqrt(pboxSize);
                    while (outputNum > 0) {
                        output.push_back(ele);
                        i++;
                        if (i == procedureH->getBlock().size()) {
                            i--;
                            break;
                        } else {
                            ele = procedureH->getBlock().at(i);
                            left = ele->getLhs();
                            if (left != nullptr and left->getOp() == ASTNode::FFTIMES and
                                left->getLhs()->getNodeName().substr(0, 5) == "pboxm") {
                                outputNum--;
                                continue;
                            } else {
                                i--;
                                left = procedureH->getBlock().at(i)->getLhs();
                                break;
                            }
                        }
                    }
                    if (outputNum == 0)
                        i--;
                    left = procedureH->getBlock().at(i)->getLhs();
                    linearPermuteGenModel(left->getLhs(), left->getRhs(), output);
                } else
                    assert(false);
            } else if (left->getOp() == ASTNode::NULLOP) {}
            else if (left->getOp() == ASTNode::SYMBOLINDEX) {}
            else
                assert(false);
        } else if (ele->getOp() == ASTNode::TOUINT) {}
        else if (ele->getOp() == ASTNode::INDEX) {}
        else if (ele->getOp() == ASTNode::ADD or ele->getOp() == ASTNode::MINUS) {
            functionCallFlag = true;
            std::string outputName = ele->getNodeName().substr(0, ele->getNodeName().find_last_of("_"));
            std::vector<ThreeAddressNodePtr> input1, input2, output;
            while (true) {
                input1.push_back(ele->getLhs());
                input2.push_back(ele->getRhs());
                output.push_back(ele);
                i++;
                if (i == procedureH->getBlock().size()) {
                    i--;
                    break;
                }
                ele = procedureH->getBlock().at(i);
                std::string  aaa = ele->getNodeName().substr(0, outputName.size());

                if (ele != nullptr and ele->getOp() == ASTNode::ADD and ele->getNodeName().substr(0, outputName.size()) == outputName)
                    continue;
                else {
                    i--;
                    break;
                }
            }
            AddGenModel(input1, input2, output);
        }
        else assert(false);

        if (!functionCallFlag) {
            for (const auto& pair : tanNameMxIndex) {
                if (ele->getLhs() != nullptr)
                    if (ele->getLhs()->getNodeName() == pair.first)
                        tanNameMxIndex[ele->getNodeName()] = pair.second;
                if (ele->getRhs() != nullptr)
                    if (ele->getRhs()->getNodeName() == pair.first)
                        tanNameMxIndex[ele->getNodeName()] = pair.second;
            }
        } else
            functionCallFlag = false;

        if (sboxFindFlag) {
            sboxFindFlag = false;
            for (const auto& name : tanNameMxIndex) {
                if (name.first == ele->getNodeName())
                    sboxIndexSave.push_back(name.second);
            }
        }
    }

    for (const auto& rtn : procedureH->getReturns()) {
        for (const auto& pair : tanNameMxIndex) {
            if (rtn->getNodeName() == pair.first) {
                rtnMxIndex[pair.first] = pair.second;
                rtnIdxSave.push_back(pair.second);
            }
        }
    }
}



