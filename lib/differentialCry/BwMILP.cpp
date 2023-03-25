//
// Created by Septi on 2/7/2023.
//
#include "BwMILP.h"

extern std::map<std::string, std::vector<int>> allBox;
extern std::map<std::string, std::vector<int>> pboxM;
extern std::map<std::string, int> pboxMSize;
extern std::map<std::string, std::vector<int>> Ffm;
extern std::string cipherName;

BwMILP::BwMILP(vector<ProcedureHPtr> procedureHs, std::string mode, int red_mode)
        : procedureHs(std::move(procedureHs)), mode(move(mode)), redMode(red_mode) {
    this->Box = allBox;
    this->cipherName = ::cipherName;
    // process various box
    auto iterator = Box.begin();
    while (iterator != this->Box.end()) {
        if (iterator->first.substr(0, 4) == "sbox") {
            std::string sboxName = iterator->first;
            SboxM sboxM(sboxName, iterator->second, this->mode);
            this->extWeighted = sboxM.get_extWeighted();
            // when redMode = 8, the ineqs is read from extern files
            std::vector<std::vector<int>> ineq_set = Red::reduction(this->redMode, sboxM);
            this->sboxIneqs[sboxName] = ineq_set;
            // check if the sbox is injective or not
            this->sboxIfInjective[sboxName] = sboxInjectiveCheck(iterator->second);
            // get the input and output size of the S-box
            sboxSizeGet(sboxName, iterator->second);
        }
        // Calculate branch number of each box
        this->branchNumMin[iterator->first] = Red::branch_num_of_sbox(iterator->second);
        iterator++;
    }
    SboxM sboxM(::cipherName);
    // for ineqs of arx-structure, we always use greedy algorithm to reduce the generated inequalities
    this->ARXineqs = Red::reduction(1, sboxM);

    // process matrix
    auto iterM = pboxM.begin();
    while (iterM != pboxM.end()) {
        BranchN branchN(iterM->second, Ffm[iterM->first], "b");
        std::vector<int> branches = branchN.getBranchNum();
        this->branchNumMin[iterM->first] = branches[0];
        this->branchNumMax[iterM->first] = branches[1];
        iterM++;
    }
}

void BwMILP::MILP() {
    compares = "xor" + std::to_string(xorConsSelect) + "_sbox" + std::to_string(sboxConsSelect) + "_matrix" + std::to_string(matrixConsSelect) + "/";

    // Initial directories
    this->modelPath = this->moPath + this->cipherName + "/" + this->mode + "/" + compares;
    this->resultsPath = this->moResults + this->cipherName + "/" + this->mode + "/" + compares;
    this->dpath += this->cipherName + "/" + this->mode + "/";
    system(("mkdir -p " + this->modelPath).c_str());
    system(("mkdir -p " + this->resultsPath).c_str());
    system(("mkdir -p " + this->dpath).c_str());
    this->finalResultsPath = this->resultsPath + this->cipherName + "_FinalResults.txt";
    system(("touch " + this->finalResultsPath).c_str());
    for (int i = this->startRound; i <= this->allSearchRounds; ++i) {
        this->currMaxSearchRound = i;
        std::cout << "\n **************** CURRENT MAX ROUOND : " << this->currMaxSearchRound << "  **************** \n" << std::endl;
        // Initial path
        this->modelPath = this->moPath + this->cipherName + "/" + this->mode + "/" + compares + std::to_string(this->currMaxSearchRound) + "_round_model.lp";
        this->resultsPath = this->moResults + this->cipherName + "/" + this->mode + "/" + compares + std::to_string(this->currMaxSearchRound) + "_round_results.txt";
        system(("touch " + this->modelPath).c_str());
        system(("touch " + this->resultsPath).c_str());
        // Differential characteristic path
        this->dcPath = this->dpath + std::to_string(this->currMaxSearchRound) + "_round_dc.txt";
        system(("touch " + this->dcPath).c_str());
        // Build models and solve them
        builder();
        solver();
        // Initial for next round
        this->tanNameMxIndex.clear();
        this->rtnMxIndex.clear();
        this->rtnIdxSave.clear();
        this->xCounter = 1;
        this->dCounter = 1;
        this->ACounter = 1;
        this->PCounter = 1;
        this->fCounter = 1;
        this->yCounter = 1;
        this->differentialCharacteristic.clear();
        // Save the current final results
        std::ofstream fResults(this->finalResultsPath, std::ios::trunc);
        if (!fResults){
            std::cout << "Wrong file path ! " << std::endl;
        } else {
            fResults << " ***************** FINAL RESULTS ***************** \n\n";
            for (int j = 0; j < this->finalResults.size(); ++j) {
                fResults << "Results Of " << j + 1 << " Rounds : \n";
                fResults << "\tObj : " << this->finalResults[j] << "\n";
                fResults << "\tclockTime : " << this->finalClockTime[j] << "\n";
                fResults << "\ttimeTime : " << this->finalTimeTime[j] << "\n\n";
            }
            fResults << " ***************** FINAL RESULTS ***************** \n";
            fResults.close();
        }
    }
    // Print final results
    std::cout << "\n ***************** FINAL RESULTS ***************** " << std::endl;
    for (int i = 0; i < this->finalResults.size(); ++i) {
        std::cout << "Results Of " << i + 1 << " Rounds : " << std::endl;
        std::cout << "\tObj : " << this->finalResults[i] << std::endl;
        std::cout << "\tclockTime : " << this->finalClockTime[i] << std::endl;
        std::cout << "\ttimeTime : " << this->finalTimeTime[i] << std::endl << std::endl;
    }
    std::cout << " ***************** FINAL RESULTS ***************** \n" << std::endl;
}

void BwMILP::builder() {
    std::ofstream scons(this->modelPath, std::ios::trunc);
    if (!scons){
        std::cout << "Wrong file path ! " << std::endl;
    } else {
        scons << "Subject To\n";
        scons.close();
    }
    int roundCounter = this->currMaxSearchRound;
    for (const auto& proc : this->procedureHs) {
        if (proc->getName() == "main") {
            std::string roundFuncId;
            bool newRoundFlag = false;
            int tempSizeCounter = 0;
            int roundFlag = false, modelCheckFlag = true;
            for (const auto& ele : proc->getBlock()) {
                // When the value of the first parameter "round" is queried,
                // it means that a round function call has already started
                if (ele->getLhs()->getNodeType() == CONSTANT) {
                    newRoundFlag = true;
                    continue;
                }
                if (newRoundFlag and roundCounter > 0) {
                    if (ele->getNodeName() == "plaintext_push")
                        tempSizeCounter++;
                    // start function call
                    if (ele->getOp() == ASTNode::CALL) {
                        this->blockSize = tempSizeCounter;
                        tempSizeCounter = 0;
                        roundFuncId = ele->getLhs()->getNodeName().substr(0, ele->getLhs()->getNodeName().find("@"));
                        for (const auto& tproc : this->procedureHs) {
                            if (tproc->getName() == roundFuncId) {
                                roundFBuilder(tproc);
                                roundFlag = true;
                                break;
                            }
                        }
                        newRoundFlag = false;
                        roundCounter--;
                    }
                }
                // If the previous model is empty, it means that the previous round will not generate the model,
                // and the related member variables need to be cleared, making sure that it does not affect the
                // subsequent model generation.
                if (roundFlag and modelCheckFlag) {
                    std::ifstream file;
                    file.open(this->modelPath);
                    std::string model, line;
                    while (getline(file, line)) {model += line + "\n";}
                    file.close();
                    if (model == "Subject To\n") {
                        // initial for next round;
                        this->tanNameMxIndex.clear();
                        this->rtnMxIndex.clear();
                        this->rtnIdxSave.clear();
                        this->xCounter = 1;
                        this->dCounter = 1;
                        this->ACounter = 1;
                        this->PCounter = 1;
                        this->fCounter = 1;
                        this->yCounter = 1;
                        roundFlag = false;
                        this->differentialCharacteristic.clear();
                    } else
                        modelCheckFlag = false;
                }
            }
            break;
        }
    }
    std::ifstream file;
    file.open(this->modelPath);
    std::string model,line;
    while (getline(file, line)) {model += line + "\n";}
    file.close();

    if (model != "Subject To\n") {
        std::ofstream target(this->modelPath, std::ios::trunc);
        if (!target) {
            std::cout << "Wrong file path ! " << std::endl;
        } else {
            target << "Minimize\n";
            if (!this->arxFlag) {
                if (this->mode == "AS") {
                    for (int i = 1; i < this->ACounter; ++i) {
                        target << "A" << i;
                        if (i != this->ACounter - 1) target << " + ";
                    }
                } else if (this->mode == "DC") {
                    for (int i = 1; i < this->PCounter; i += extWeighted.size()) {
                        int k = 0;
                        for (auto weight: extWeighted) {
                            if (weight != 1)
                                target << weight << " P" << i + k;
                            else
                                target << "P" << i + k;
                            if (i + k != this->PCounter - 1) target << " + ";
                            k++;
                        }
                    }
                }
            } else {
                if (!this->ARXlinearRem.empty()) {
                    for (int i = 1; i < this->PCounter; ++i) {
                        if (std::find(this->ARXlinearRem.begin(), this->ARXlinearRem.end(),i) == this->ARXlinearRem.end()) {
                            target << "P" << i;
                            if (i != this->PCounter - 2) target << " + ";
                        } else
                            continue;
                    }
                } else {
                    for (int i = 1; i < this->PCounter; ++i) {
                        target << "P" << i;
                        if (i != this->PCounter - 1) target << " + ";
                    }
                }
            }
            target << "\n";
            target << model;
            // Here we need to add a constraint, since there may be modeling of zeroing after shift in the subsequent matrix,
            // and all the zeroing bits are represented by x_0, hence we initial the value of x_0 to 0 here
            target << "x0 = 0\n";
            for (int i = 0; i < blockSize; ++i) {
                if (i != blockSize - 1)
                    target << "x" << std::to_string(i + 1) << " + ";
                else
                    target << "x" << std::to_string(i + 1);
            }
            target << " >= 1\n";
        }
        target.close();
    }
    std::ofstream binary(this->modelPath, std::ios::app);
    if (!binary) {
        std::cout << "Wrong file path ! " << std::endl;
    } else {
        binary << "Binary\n";
        // ILP or MILP ?
        if (this->ILPFlag) {
            for (int i = 1; i < this->xCounter; ++i)
                binary << "x" << i << "\n";
        } else {
            for (int i = 1; i < blockSize + 1; ++i) {
                binary << "x" << i << "\n";
            }
        }
        for (int i = 1; i < this->dCounter; ++i)
            binary << "d" << i << "\n";
        if (!this->arxFlag) {
            if (this->mode == "AS") {
                for (int i = 1; i < this->ACounter; ++i)
                    binary << "A" << i << "\n";
            } else if (this->mode == "DC") {
                for (int i = 1; i < this->PCounter; ++i)
                    binary << "P" << i << "\n";
            }
        } else {
            for (int i = 0; i < this->PCounter; ++i)
                binary << "P" << i << "\n";
        }
        for (int i = 1; i < this->fCounter; ++i)
            binary << "f" << i << "\n";
        // integer variables
        binary << "General\n";
        for (int i = 0; i < this->yCounter; ++i)
            binary << "y" << i << "\n";
        binary << "End";
        binary.close();
    }
}

void BwMILP::roundFBuilder(const ProcedureHPtr &procedureH) {
    std::string roundId, keyId, plaintextId;
    roundId = procedureH->getParameters().at(0).at(0)->getNodeName();
    this->roundID = roundId;
    keyId = procedureH->getParameters().at(1).at(0)->getNodeName().substr(0, procedureH->getParameters().at(1).at(0)->getNodeName().find("0"));
    plaintextId = procedureH->getParameters().at(2).at(0)->getNodeName().substr(0, procedureH->getParameters().at(2).at(0)->getNodeName().find("0"));

    // At the beginning of each round, we need to store the result of the previous round of "return" to xIndex,
    // and the three address instances corresponding to each index are the input plaintext of the round function
    if (!this->rtnIdxSave.empty()) {
        for (int i = 0; i < this->rtnIdxSave.size(); ++i)
            this->tanNameMxIndex[procedureH->getParameters().at(2).at(i)->getNodeName()] = this->rtnIdxSave[i];
        this->rtnIdxSave.clear();
        this->rtnMxIndex.clear();
        // save input difference
        std::vector<int> inputC;
        for (int & i : this->rtnIdxSave) {
            inputC.push_back(i);
        }
        this->differentialCharacteristic.push_back(inputC);
    } else {
        for (const auto & i : procedureH->getParameters().at(2)) {
            this->tanNameMxIndex[i->getNodeName()] = this->xCounter;
            this->xCounter++;
        }
        // initial input difference
        std::vector<int> inputC;
        for (int i = 1; i < this->xCounter; ++i) {
            inputC.push_back(i);
        }
        this->differentialCharacteristic.push_back(inputC);
    }

    bool functionCallFlag;
    for (int i = 0; i < procedureH->getBlock().size(); ++i) {
        functionCallFlag = false;
        ThreeAddressNodePtr ele = procedureH->getBlock().at(i);
        if (ele->getOp() == ASTNode::XOR) {
            // XOR with keys or constants does not affect differential propagation
            if (ele->getLhs()->getNodeName().find(keyId) != std::string::npos or
                ele->getRhs()->getNodeName().find(keyId) != std::string::npos) {}
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
                functionCallFlag = true;
                /*
                 * There are two kind of cases:
                 * 1. The input and output are Uints, which is not an array type and can be processed directly as input
                 * 2. The input and output are arrays, which need to be processed as arrays
                 * */
                if (ele->getNodeType() != NodeType::UINT1) {
                    // 因为对touint的处理时，会将touint和后续symbolindex同时存放sbox操作，所以会重复操作两次
                    // 那么在symbolindex之外的sbox操作就不作处理了。
                    //UintsSboxGenModel(ele->getLhs(), ele->getRhs(), ele);
                }
                else {
                    std::vector<ThreeAddressNodePtr> output;
                    int outputNum = log2(allBox[ele->getLhs()->getNodeName()].size());
                    while (outputNum > 0) {
                        output.push_back(ele);
                        i++;
                        if (i == procedureH->getBlock().size()) {
                            i--;
                            break;
                        }
                        ele = procedureH->getBlock().at(i);
                        if (ele != nullptr and ele->getOp() == ASTNode::BOXOP and ele->getLhs()->getNodeName().substr(0, 4) == "pbox") {
                            outputNum--;
                            continue;
                        } else {
                            i--;
                            ele = procedureH->getBlock().at(i);
                            break;
                        }
                    }
                    if (outputNum == 0) i--;
                    Uint1SboxGenModel(ele->getLhs(), ele->getRhs(), output);
                }
            } else if (ele->getLhs()->getNodeName().substr(0, 4) == "pbox") {
                functionCallFlag = true;
                // Only Uint1 types are processed in Bit-wise MILP
                if (ele->getNodeType() != NodeType::UINT1)
                    assert(false);
                else {
                    // The situation that the number of inputs and outputs is not the same is not considered for the time being
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
                    Uint1IdxPboxPermute(ele->getLhs(), ele->getRhs(), output);
                }
            }
        }
        // When the operator is SYMBOLINDEX, it means that the object being processed is an array whose elements are input
        // dependent and cannot be evaluated as a concrete value
        else if (ele->getOp() == ASTNode::SYMBOLINDEX) {
            /*
             * Remark：
             * 本来我考虑的是，有的操作可能是复合了多种操作的组合，比如，a[0] = sbox<sbox_int>[3];
             * 这个操作需要先进行BOXOP，然后是SYMBOLINDEX，
             * 此时，需要访问SYMBOLINDEX的左孩子节点，并根据它的操作类型是否是BOXOP来选择相应的约束生成函数。
             * 但是实际上在分析时，BOXOP是block中的前一个元素，即在分析SYMBOLINDEX为操作的sequence之前，BOXOP已经被处理过了。
             * 这样我们在SYMBOLINDEX时再考虑BOXOP，会出现重复处理的情况，
             * 所以既然我们在三地址时已经将所有的操作符都拆分了，即每个sequence都只有一个op，其余的op需要继续访问孩子节点，
             * 那么我们不需要关心具体孩子节点的op，因为会在处理本语句之前，就先把孩子节点需要处理的已经完成了。
             *
             * 但是我们还有一种情况是目前无法区分的，即涉及到数组处理时，孩子节点是BOXOP，本节点是SYMBOLINDEX，
             * 此时孩子节点是没有被进行处理的，我们该如何进行这样的区分呢？
             * Remark in 2022.10.24
             *  当声明一个数组，其中每个元素都是进行BOXOP时，就需要根据其孩子节点的操作类型来进行对应的分析
             *  因为SYMBOLINDEX的对应index在右孩子节点，所以只分析其左孩子节点的操作类型就可以
             * */
            ThreeAddressNodePtr left = ele->getLhs();
            if (left->getOp() == ASTNode::BOXOP) {
                if (left->getLhs()->getNodeName().substr(0, 4) == "sbox") {
                    functionCallFlag = true;
                    if (ele->getNodeType() != NodeType::UINT1)
                        UintsSboxGenModel(left->getLhs(), left->getRhs(), ele);
                    else {
                        std::vector<ThreeAddressNodePtr> output;
                        int outputNum = log2(allBox[left->getLhs()->getNodeName()].size());
                        while (outputNum > 0) {
                            output.push_back(ele);
                            i++;
                            if (i == procedureH->getBlock().size()) {
                                i--;
                                break;
                            } else {
                                ele = procedureH->getBlock().at(i);
                                left = ele->getLhs();
                                if (left != nullptr and left->getOp() == ASTNode::BOXOP and
                                    left->getLhs()->getNodeName().substr(0, 4) == "sbox") {
                                    outputNum--;
                                    continue;
                                } else {
                                    i--;
                                    left = procedureH->getBlock().at(i)->getLhs();
                                    break;
                                }
                            }
                        }
                        // m_out1_12
                        if (outputNum == 0)
                            i--;
                        left = procedureH->getBlock().at(i)->getLhs();
                        Uint1SboxGenModel(left->getLhs(), left->getRhs(), output);
                    }
                } else if (left->getLhs()->getNodeName().substr(0, 4) == "pbox") {
                    functionCallFlag = true;
                    if (ele->getNodeType() != NodeType::UINT1) assert(false);
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
                                if (left != nullptr and left->getOp() == ASTNode::BOXOP and
                                    left->getLhs()->getNodeName() == pboxName)
                                    continue;
                                else {
                                    i--;
                                    left = procedureH->getBlock().at(i)->getLhs();
                                    break;
                                }
                            }
                        }
                        Uint1IdxPboxPermute(left->getLhs(), left->getRhs(), output);
                    }
                } else
                    assert(false);
            }
            // Ffm
            else if (left->getOp() == ASTNode::FFTIMES) {
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
                    if (outputNum == 0) i--;
                    left = procedureH->getBlock().at(i)->getLhs();
                    MatrixVectorGenModel(left->getLhs(), left->getRhs(), output);
                } else
                    assert(false);
            }
            // Do nothing if operator is NULLOP
            else if (left->getOp() == ASTNode::NULLOP) {}
            else if (left->getOp() == ASTNode::SYMBOLINDEX) {}
            else
                assert(false);
        } else if (ele->getOp() == ASTNode::TOUINT) {}
        else if (ele->getOp() == ASTNode::INDEX) {}
        // This operator currently only appears in ARX structures
        else if (ele->getOp() == ASTNode::ADD) {
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
            Uint1AddMinusGenModel(input1, input2, output, 1);
        } else if (ele->getOp() == ASTNode::MINUS) {
            // 目前发现的情况是有关轮数r，即第一个参数的减法。
            // 此时，因为轮数r是可以evaluate的，因为在传参时，r是确定的，所以需要在这里evaluate结果
            // 暂时未确定，后续需要check所有的benchmark情况

            if (isConstant(ele->getLhs()) or isConstant(ele->getRhs())) {

            } else {
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
                Uint1AddMinusGenModel(input1, input2, output, 2);
            }
        }
        // Throws an error when an uncovered condition is found
        else
            assert(false);

        // If no function call is performed, it means that this node does not participate in any constraint generation operation.
        // At this time, if the left and right child nodes have been stored in xIndex, we need to overwrite their original nodes,
        // because subsequent operations need to be carried out on their basis.
        // Here we still use name of node to judge, because different pointers may point to the same memory address.
        if (!functionCallFlag) {
            if ((ele->getLhs() == nullptr or
                 (ele->getLhs()->getNodeType() != NodeType::UINT1 and ele->getLhs()->getNodeType() != NodeType::PARAMETER) )
                and
                (ele->getRhs() == nullptr or
                (ele->getRhs()->getNodeType() != NodeType::UINT1  and ele->getRhs()->getNodeType() != NodeType::PARAMETER))
                and
                    (ele->getOp() != ASTNode::TOUINT)) {
                std::string finderName = ele->getLhs()->getNodeName() + "_B" + ele->getRhs()->getNodeName();
                for (const auto &pair: this->tanNameMxIndex) {
                    if (finderName == pair.first)
                        this->tanNameMxIndex[ele->getNodeName()] = pair.second;
                }
            } else {
                for (const auto &pair: this->tanNameMxIndex) {
                    if (ele->getLhs() != nullptr)
                        if (ele->getLhs()->getNodeName() == pair.first)
                            this->tanNameMxIndex[ele->getNodeName()] = pair.second;
                    if (ele->getRhs() != nullptr)
                        if (ele->getRhs()->getNodeName() == pair.first)
                            this->tanNameMxIndex[ele->getNodeName()] = pair.second;
                }
            }
        } else
            functionCallFlag = false;
    }

    for (const auto& rtn : procedureH->getReturns()) {
        for (const auto &pair: this->tanNameMxIndex) {
            if (rtn->getNodeName() == pair.first) {
                this->rtnMxIndex[pair.first] = pair.second;
                this->rtnIdxSave.push_back(pair.second);
            }
        }
    }

    // output difference
    std::vector<int> outputC;
    for (int & i : this->rtnIdxSave) {
        outputC.push_back(i);
    }
    this->differentialCharacteristic.push_back(outputC);
}

void BwMILP::XORGenModel(const ThreeAddressNodePtr &left, const ThreeAddressNodePtr &right,
                         const ThreeAddressNodePtr &result) {
    int inputIdx1 = 0, inputIdx2 = 0;
    for (const auto& pair : this->tanNameMxIndex) {
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
                item = this->xCounter;
                this->tanNameMxIndex[left->getNodeName()] = item;
                this->xCounter++;
            }
        }
    }

    int outputIdx = this->xCounter;
    this->tanNameMxIndex[result->getNodeName()] = outputIdx;
    this->xCounter++;
    switch (this->xorConsSelect) {
        case 1:
            MILPcons::bXorC1(this->modelPath, inputIdx[0], inputIdx[1], outputIdx, this->dCounter);
            break;
        case 2:
            MILPcons::bXorC2(this->modelPath, inputIdx[0], inputIdx[1], outputIdx);
            break;
        case 3:
            MILPcons::bXorC3(this->modelPath, inputIdx[0], inputIdx[1], outputIdx, this->dCounter);
            break;
        default:
            MILPcons::bXorC2(this->modelPath, inputIdx[0], inputIdx[1], outputIdx);
            break;
    }
}

void BwMILP::ANDandORGenModel(const ThreeAddressNodePtr &left, const ThreeAddressNodePtr &right,
                              const ThreeAddressNodePtr &result) {
    int inputIdx1 = 0, inputIdx2 = 0;
    for (const auto& pair : this->tanNameMxIndex) {
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
                item = this->xCounter;
                this->tanNameMxIndex[left->getNodeName()] = item;
                this->xCounter++;
            }
        }
    }

    int outputIdx = this->xCounter;
    this->tanNameMxIndex[result->getNodeName()] = outputIdx;
    this->xCounter++;
    if (mode == "AS")
        MILPcons::bAndOrC(this->modelPath, inputIdx[0], inputIdx[1], outputIdx);
    else if (mode == "DC")
        MILPcons::dAndOrC(this->modelPath, inputIdx[0], inputIdx[1], outputIdx, this->PCounter);
    else
        assert(false);
}

void BwMILP::UintsSboxGenModel(const ThreeAddressNodePtr &sbox, const ThreeAddressNodePtr &input,
                               const ThreeAddressNodePtr &output) {
// Determine the input variable of the sbox generation constraint based on the type of input
    std::vector<int> inputIdx = extIdxFromTOUINTorBOXINDEX(input);
    /*
     * Determine the output variable of the sbox generation constraint based on the type of output
     *
     * Because toUint is of type Uints, there should be multiple corresponding output bits at this time when actually modeling.
     * But there is only one instance of ThreeaddressNodePtr, so at this time we no longer have a one-to-one correspondence
     * between xCounter and ThreeaddressNode in xIndex. Instead, only the correspondence between the last xCounter and ThressaddressNode
     * is recorded. Therefore, we directly build the corresponding model here, and record the mapping relationship in the last
     * index of output.
     */
    // here the outputSize should be obtained via the member variable sboxOutputSize
    //int outputSize = Transformer::getNodeTypeSize(output->getNodeType());
    int outputSize = sboxOutputSize[sbox->getNodeName()];
    std::vector<int> outputIdx;
    while (outputSize > 0) {
        outputIdx.push_back(this->xCounter);
        this->tanNameMxIndex[output->getNodeName()] = this->xCounter;
        this->xCounter++;
        outputSize--;
    }
    if (this->mode == "AS") {
        switch (this->sboxConsSelect) {
            case 1:
                MILPcons::bSboxC1(this->modelPath, inputIdx, outputIdx, this->sboxIneqs[sbox->getNodeName()],
                                  this->branchNumMin[sbox->getNodeName()], this->dCounter, this->ACounter, sboxIfInjective[sbox->getNodeName()]);
                break;
            case 2:
                MILPcons::bSboxC2(this->modelPath, inputIdx, outputIdx, this->sboxIneqs[sbox->getNodeName()],
                                  this->ACounter, sboxIfInjective[sbox->getNodeName()]);
                break;
            case 3:
                MILPcons::bSboxC3(this->modelPath, inputIdx, outputIdx, this->sboxIneqs[sbox->getNodeName()],
                                  this->ACounter);
                break;
            default:
                MILPcons::bSboxC2(this->modelPath, inputIdx, outputIdx, this->sboxIneqs[sbox->getNodeName()],
                                  this->ACounter, sboxIfInjective[sbox->getNodeName()]);
        }
    } else if (this->mode == "DC") {
        switch (this->sboxConsSelect) {
            case 1:
                MILPcons::dSboxC1(this->modelPath, inputIdx, outputIdx, this->sboxIneqs[sbox->getNodeName()],
                                  this->branchNumMin[sbox->getNodeName()], this->dCounter, this->PCounter, sboxIfInjective[sbox->getNodeName()], this->extWeighted.size());
                break;
            case 2:
                MILPcons::dSboxC2(this->modelPath, inputIdx, outputIdx, this->sboxIneqs[sbox->getNodeName()],
                                  this->PCounter, sboxIfInjective[sbox->getNodeName()], this->extWeighted.size());
                break;
            case 3:
                MILPcons::dSboxC3(this->modelPath, inputIdx, outputIdx, this->sboxIneqs[sbox->getNodeName()],
                                  this->PCounter, this->extWeighted.size());
                break;
            default:
                MILPcons::dSboxC2(this->modelPath, inputIdx, outputIdx, this->sboxIneqs[sbox->getNodeName()],
                                  this->PCounter, sboxIfInjective[sbox->getNodeName()], this->extWeighted.size());
        }
    } else
        assert(false);
}

void BwMILP::Uint1SboxGenModel(const ThreeAddressNodePtr &sbox, const ThreeAddressNodePtr &input,
                               const vector<ThreeAddressNodePtr> &output) {
    std::vector<int> inputIdx = extIdxFromTOUINTorBOXINDEX(input);
    std::vector<int> outputIdx;
    for (const auto& ele : output) {
        this->tanNameMxIndex[ele->getNodeName()] = this->xCounter;
        outputIdx.push_back(this->xCounter);
        this->xCounter++;
    }
    if (this->mode == "AS") {
        switch (this->sboxConsSelect) {
            case 1:
                MILPcons::bSboxC1(this->modelPath, inputIdx, outputIdx, this->sboxIneqs[sbox->getNodeName()],
                                  this->branchNumMin[sbox->getNodeName()], this->dCounter, this->ACounter, sboxIfInjective[sbox->getNodeName()]);
                break;
            case 2:
                MILPcons::bSboxC2(this->modelPath, inputIdx, outputIdx, this->sboxIneqs[sbox->getNodeName()],
                                  this->ACounter, sboxIfInjective[sbox->getNodeName()]);
                break;
            case 3:
                MILPcons::bSboxC3(this->modelPath, inputIdx, outputIdx, this->sboxIneqs[sbox->getNodeName()],
                                  this->ACounter);
                break;
            default:
                MILPcons::bSboxC2(this->modelPath, inputIdx, outputIdx, this->sboxIneqs[sbox->getNodeName()],
                                  this->ACounter, sboxIfInjective[sbox->getNodeName()]);
        }
    } else if (this->mode == "DC") {
        switch (this->sboxConsSelect) {
            case 1:
                MILPcons::dSboxC1(this->modelPath, inputIdx, outputIdx, this->sboxIneqs[sbox->getNodeName()],
                                  this->branchNumMin[sbox->getNodeName()], this->dCounter, this->PCounter, sboxIfInjective[sbox->getNodeName()],
                                  this->extWeighted.size());
                break;
            case 2:
                MILPcons::dSboxC2(this->modelPath, inputIdx, outputIdx, this->sboxIneqs[sbox->getNodeName()],
                                  this->PCounter, sboxIfInjective[sbox->getNodeName()], this->extWeighted.size());
                break;
            case 3:
                MILPcons::dSboxC3(this->modelPath, inputIdx, outputIdx, this->sboxIneqs[sbox->getNodeName()],
                                  this->PCounter, this->extWeighted.size());
                break;
            default:
                MILPcons::dSboxC2(this->modelPath, inputIdx, outputIdx, this->sboxIneqs[sbox->getNodeName()],
                                  this->PCounter, sboxIfInjective[sbox->getNodeName()], this->extWeighted.size());
        }
    } else
        assert(false);
}

void BwMILP::Uint1IdxPboxPermute(const ThreeAddressNodePtr &pbox, const ThreeAddressNodePtr &input,
                                 const vector<ThreeAddressNodePtr> &output) {
    std::vector<int> pboxValue = this->Box[pbox->getNodeName()];
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
        for (const auto& pair : this->tanNameMxIndex) {
            if (pair.first == i->getNodeName()) {
                inputIdx.push_back(pair.second);
                iFlag = true;
            }
        }
        if (!iFlag) {
            inputIdx.push_back(this->xCounter);
            this->xCounter++;
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
        this->tanNameMxIndex[output.at(i)->getNodeName()] = outputIdx[i];
}

void BwMILP::MatrixVectorGenModel(const ThreeAddressNodePtr &pboxm, const ThreeAddressNodePtr &input,
                                  const vector<ThreeAddressNodePtr> &output) {
    /*
     * 这里我们梳理一下整个矩阵乘法的处理流程：
     *
     * 1. 我们描述的应该是所有输入和所有输出之间的差分传播关系，因为输出 vector 中的每个 y_i 之间的结果互相并不影响，
     *    所以我们可以单独考虑每个 y_i 的传播过程;
     *
     * 2. 输出vector的每个元素 y_i 应该是输入vector中所有 x_j 与对应第 i 行的每个元素 M_ij 依次进行有限域乘法的结
     *    果的异或， 因此，对于每个输出元素 y_i，都要去搜集影响其映射的输入元素 x_j;
     *
     * 3. 对于每个影响输出 y_i 的输入元素 x_j， 其都要先与矩阵 M 中的元素 M_ij 进行有限域的乘法，我们需要将该乘法转
     *    换乘 移位 加 异或，取模 的运算， 与 x_j 与 M_ij 的计算结果之间再两两进行异或，得到 y_i;
     *
     * 4. 某个有限域乘法中，模的结果就是 2^(n-1) * 2 的值， 如： AES的模就是 10000000 * 00000010
     *
     * 实现逻辑：
     * 1. 对于每个 y_i， 维护一个 map， 其保存正对应 y_i 的所有 x_j;
     *
     * 2. 根据 x_j 对应操作的 M_ij, 将有限域乘法转换成 移位，异或，取模（异或模）的操作， 并将所有需要进行异或的值，
     *    用一个map保存起来，映射到 y_i.
     *    此时， 每个 y_i 应该都映射到了其对应所有 x_j 下，生成的需要两两进行异或的所有值.
     *
     *    注：此时的操作需要区分 uint1 与 uints：
     *      1） 对于 uint1，可以直接根据常数项是否为0来判断 y_i 映射到的 x_j 集合
     *      2） 对于 uints, 需要根据与常数项有限域乘法的拆分结果，来得到 y_i 映射到的所有集合元素， 当某个常数项不为
     *          0时，对应 x_j 应该会生成一个或多个元素（y_i的映射集合中的元素）
     *
     * 3. 根据初始输入 x_j 和 最终输出 y_i 构造约束
     *
     * */

    // 提取所有input
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

    // 初始化 matrix 和 ffm, 这里应该直接吧 matrix 和 ffm 直接用二进制字符串表示
    std::vector<std::vector<int>> Matrix; // matrix
    std::vector<int> pboxmValue = pboxM[pboxm->getNodeName()];
    int rowSize = inputTAN.size(), rowCounter = 0;
    std::vector<int> tempRow;
    // Reassemble pboxm into a matrix
    for (int & i : pboxmValue) {
        tempRow.push_back(i);
        rowCounter++;
        if (rowCounter == rowSize) {
            Matrix.push_back(tempRow);
            tempRow.clear();
            rowCounter = 0;
        }
    }
    tempRow.clear();
    rowCounter = 0;
    std::vector<int> ffmValue = Ffm[pboxm->getNodeName()];
    std::vector<std::vector<int>> Ffm; // ffm
    int ffmRowSize = int(sqrt(ffmValue.size())); // 因为ffm一定是个方阵，所以求平方根以后就是ffm每行的size
    for (int & i : ffmValue) {
        tempRow.push_back(i);
        rowCounter++;
        if (rowCounter == ffmRowSize) {
            Ffm.push_back(tempRow);
            tempRow.clear();
            rowCounter = 0;
        }
    }

    // 区分 uint1 和 uints
    int eleSize;
    if (inputTAN[0]->getNodeType() == NodeType::PARAMETER)
        eleSize = 1;
    else
        eleSize = transNodeTypeSize(inputTAN[0]->getNodeType());

    // 根据 elesize 分别进行处理
    // uints
    if (eleSize != 1) {
        // 二进制字符串表示数组
        std::vector<std::vector<std::string>> MatrixStr;
        for (const auto& row : Matrix) {
            std::vector<std::string> tt;
            for (auto ele: row) {
                // 先将FFm中的每个元素转化为长度为sizeS的二进制字符串
                std::string eleB = std::to_string(utilities::d_to_b(ele));
                for (int i = eleB.size(); i < eleSize; ++i)
                    eleB = "0" + eleB;
                tt.push_back(eleB);
            }
            MatrixStr.push_back(tt);
        }
        // 有限域的模
        int module = Ffm[2][int(pow(2, (eleSize - 1)))];
        std::string moduleStr = std::to_string(utilities::d_to_b(module));
        for (int i = moduleStr.size(); i < eleSize; ++i)
            moduleStr = "0" + moduleStr;
        std::vector<int> moduleVec;
        for (char i : moduleStr) {
            if (i == '0') moduleVec.push_back(0);
            else if (i == '1') moduleVec.push_back(1);
        }

        std::map<ThreeAddressNodePtr, std::vector<ThreeAddressNodePtr>> MoutMap; // y_i -> x_j
        std::map<ThreeAddressNodePtr, std::vector<std::string>> MoutMapMatrix; // y_i -> M_ij
        // 有限域乘法描述约束映射
        for (int i = 0; i < output.size(); ++i) {
            std::vector<ThreeAddressNodePtr> MoutEleMap;
            std::vector<std::string> MoutEleMapMatrix;
            for (int j = 0; j < rowSize; ++j) {
                MoutEleMap.push_back(inputTAN[j]);
                MoutEleMapMatrix.push_back(MatrixStr[i][j]);
            }
            MoutMap[output[i]] = MoutEleMap;
            MoutMapMatrix[output[i]] = MoutEleMapMatrix;
        }

        std::map<ThreeAddressNodePtr, std::vector<int>> MoutIdxMap; // y_i -> indexes
        std::map<ThreeAddressNodePtr, std::vector<int>> MintIdxMap; // x_j -> indexes
        for (auto ele : inputTAN) {
            std::vector<int> inputIdx = extIdxFromTOUINTorBOXINDEX(ele);
            MintIdxMap[ele] = inputIdx;
        }
        for (auto ele : output) {
            std::vector<int> outputIdx;
            for (int i = 0; i < eleSize; ++i) {
                this->tanNameMxIndex[ele->getNodeName() + "_B" + std::to_string(i)] = this->xCounter;
                outputIdx.push_back(this->xCounter);
                this->xCounter++;
            }
            MoutIdxMap[ele] = outputIdx;
        }

        // 构造每个 y_i 对应所有 x_j 下，需要两两进行异或的所有值的映射
        std::map<std::vector<int>, std::vector<std::vector<int>>> MoutIdxIntIdxes;
        // 每个 y_i 对应下可能的溢出输入位
        std::map<std::vector<int>, std::vector<int>> MoutIdxOverflowIdxes;

        auto item = MoutMap.begin();
        auto itemMoutMatrix = MoutMapMatrix.begin();
        for (int i = 0; i < MoutMap.size(); ++i) {
            // 每个 y_i 映射的所有值
            std::vector<std::vector<int>> tMapIdxes;
            std::vector<int> tOverflows; // 可能溢出的 x_j 中的某一位, 这里直接保存对应的index
            // 对 matrix 中每一行的元素，都应该与 MoutMap 中对应的元素一一对应
            for (int j = 0; j < itemMoutMatrix->second.size(); ++j) {
                std::vector<int> x_i = MintIdxMap[item->second[j]];
                std::string M_ij = itemMoutMatrix->second[j];
                int shiftNum = 0; // 移位数量

                for (int k = 0; k < eleSize; ++k) {
                    // 对于每个 M_ij，其每一位都是确定的，可以用来判断对应的 x_j 移位数目，但是如何判断 x_j 移位后是否溢出呢？
                    // 尝试：
                    // gurobi 支持 indicator constraint, 可以使用左移移出的 x_jk 作为表示 binary，用于标明是否要异或 模向量
                    if (M_ij[eleSize - 1 - k] == '1') {
                        // 这里添加可能溢出位时还要判断是否里面已经有了，如果有该位，则需要消除。
                        // 因为与模的偶数次异或会抵消
                        if (k > 0) {
                            for (int l = 0; l < k; ++l) {
                                auto finder = std::find(tOverflows.begin(), tOverflows.end(), x_i[l]);
                                if (finder != tOverflows.end())
                                    tOverflows.erase(finder);
                                else
                                    tOverflows.push_back(x_i[l]);
                            }

                            // 这里应该给一个用于shift的函数，接受一个vector和移位的数目，返回移位后的vector
                            tMapIdxes.push_back(shiftVec(x_i, k));
                        } else {
                            // 不用移位，直接放入 y_i 对应映射
                            tMapIdxes.push_back(x_i);
                        }
                    } else if (M_ij[eleSize - 1 - k] == '0')
                        continue;
                    shiftNum++;
                }
            }
            MoutIdxIntIdxes[MoutIdxMap[item->first]] = tMapIdxes;
            MoutIdxOverflowIdxes[MoutIdxMap[item->first]] = tOverflows;
            item++;
            itemMoutMatrix++;
        }

        // 提取出每个 y_i 映射的 xor 操作对象集合， 和 y_i 映射的溢出位集合，
        // 根据两个集合生成约束

        // 需要在 溢出位集合中所有元素异或结果为1时， 增加和 模 的异或
        // 新建一个变量 f 作为溢出位异或的结果，然后根据 f 的值为 0 还是 1 来判断是否要和 模 进行异或, 然后输出最终结果 y_end
        // if f == 0 : y_end = y_i    ( y_end - y_i = 0 )
        // if f == 1 : y_end = - y_i    ( y_end + y_i = 0 )
        auto itemMidx = MoutIdxIntIdxes.begin();
        auto itemMove = MoutIdxOverflowIdxes.begin();
        for (int i = 0; i < MoutIdxIntIdxes.size(); ++i) {
            if (matrixConsSelect == 1 or matrixConsSelect == 2) {
                MILPcons::bMatrixEntryC12(this->modelPath, itemMidx->second, itemMidx->first, itemMove->second, moduleVec,
                                         this->xCounter, this->yCounter, this->fCounter, this->dCounter, this->matrixConsSelect);
            } else if (matrixConsSelect == 2) {
                MILPcons::bMatrixEntryC3(this->modelPath, itemMidx->second, itemMidx->first, itemMove->second, moduleVec,
                                         this->xCounter, this->yCounter, this->fCounter);
            }
            itemMidx++;
            itemMove++;
        }
    } else {
        // 对 uint1 的处理需要延用之前的方法
        // rowSize即矩阵的列数，最终矩阵乘法的结果是 和列数相同数目的矩阵 按相对应位进行XOR，所以我们需要先得到rowSize数目的vector，其中每个元素都是最终要进行XOR的vector
        std::map<ThreeAddressNodePtr, std::vector<ThreeAddressNodePtr>> MoutMap;
        for (int i = 0; i < output.size(); ++i) {
            std::vector<ThreeAddressNodePtr> MoutEleMap;
            for (int j = 0; j < rowSize; ++j) {
                if (Ffm[Matrix[i][j]][1] == 1) {
                    MoutEleMap.push_back(inputTAN[j]);
                }
            }
            MoutMap[output[i]] = MoutEleMap;
        }

        for (auto pair : MoutMap) {
            if (pair.second.size() >= 2) {
                if (this->matrixConsSelect == 1 or this->matrixConsSelect == 2) {
                    multiXORGenModel12(pair.second, pair.first);
                } else if (this->matrixConsSelect == 3) {
                    multiXORGenModel3(pair.second, pair.first);
                }
            }
            else {
                int rrr = this->tanNameMxIndex[pair.second.at(0)->getNodeName()];
                this->tanNameMxIndex[pair.first->getNodeName()] = this->tanNameMxIndex[pair.second.at(0)->getNodeName()];
            }
        }
    }
}

void BwMILP::multiXORGenModel12(std::vector<ThreeAddressNodePtr> input, ThreeAddressNodePtr result) {
    ThreeAddressNodePtr input1 = input[0], input2 = input[1];
    int inputIdx1 = 0, inputIdx2 = 0;
    for (const auto& pair : this->tanNameMxIndex) {
        if (pair.first == input1->getNodeName())
            inputIdx1 = pair.second;
        if (pair.first == input2->getNodeName())
            inputIdx2 = pair.second;
    }
    if (inputIdx1 == 0) {
        if (!this->rtnIdxSave.empty()) {
            inputIdx1 = this->rtnIdxSave.front();
            this->rtnIdxSave.erase(this->rtnIdxSave.cbegin());
            this->tanNameMxIndex[input1->getNodeName()] = inputIdx1;
        } else {
            inputIdx1 = this->xCounter;
            this->tanNameMxIndex[input1->getNodeName()] = inputIdx1;
            this->xCounter++;
        }
    }
    if (inputIdx2 == 0) {
        if (!this->rtnIdxSave.empty()) {
            inputIdx2 = this->rtnIdxSave.front();
            rtnIdxSave.erase(this->rtnIdxSave.cbegin());
            this->tanNameMxIndex[input2->getNodeName()] = inputIdx2;
        } else {
            inputIdx2 = this->xCounter;
            this->tanNameMxIndex[input2->getNodeName()] = inputIdx2;
            this->xCounter++;
        }
    }

    int outputIdx = this->xCounter;
    this->xCounter++;
    if (input.size() == 2)
        this->tanNameMxIndex[result->getNodeName()] = outputIdx;
    switch (matrixConsSelect) {
        case 1:
            MILPcons::bXorC1(this->modelPath, inputIdx1, inputIdx2, outputIdx, this->dCounter);
            break;
        case 2:
            MILPcons::bXorC2(this->modelPath, inputIdx1, inputIdx2, outputIdx);
            break;
        default:
            MILPcons::bXorC2(this->modelPath, inputIdx1, inputIdx2, outputIdx);
    }

    // Process the remaining elements
    if (input.size() > 2) {
        for (int i = 2; i < input.size(); ++i) {
            inputIdx1 = outputIdx;
            ThreeAddressNodePtr inputNew = input[i];
            for (const auto& pair : this->tanNameMxIndex)
                if (pair.first == inputNew->getNodeName())
                    inputIdx2 = pair.second;

            if (inputIdx2 == 0) {
                if (!this->rtnIdxSave.empty()) {
                    inputIdx2 = this->rtnIdxSave.front();
                    this->rtnIdxSave.erase(this->rtnIdxSave.cbegin());
                    this->tanNameMxIndex[inputNew->getNodeName()] = inputIdx2;
                } else {
                    inputIdx2 = this->xCounter;
                    this->tanNameMxIndex[inputNew->getNodeName()] = inputIdx2;
                    this->xCounter++;
                }
            }

            outputIdx = this->xCounter;
            this->xCounter++;
            if (i == input.size() - 1)
                this->tanNameMxIndex[result->getNodeName()] = outputIdx;
            switch (matrixConsSelect) {
                case 1:
                    MILPcons::bXorC1(this->modelPath, inputIdx1, inputIdx2, outputIdx, this->dCounter);
                    break;
                case 2:
                    MILPcons::bXorC2(this->modelPath, inputIdx1, inputIdx2, outputIdx);
                    break;
                default:
                    MILPcons::bXorC2(this->modelPath, inputIdx1, inputIdx2, outputIdx);
            }
        }
    }
}

void BwMILP::multiXORGenModel3(std::vector<ThreeAddressNodePtr> input, ThreeAddressNodePtr result) {
    std::vector<int> inputIdx;
    for (auto ele : input) {
        bool finder = false;
        for (const auto& pair : this->tanNameMxIndex) {
            if (pair.first == ele->getNodeName()) {
                inputIdx.push_back(pair.second);
                finder = true;
            }
        }
        if (!finder) {
            inputIdx.push_back(0);
        }
    }

    for (int i = 0; i < inputIdx.size(); ++i) {
        if (inputIdx[i] == 0) {
            if (!this->rtnIdxSave.empty()) {
                inputIdx[i] = this->rtnIdxSave.front();
                this->rtnIdxSave.erase(this->rtnIdxSave.cbegin());
                this->tanNameMxIndex[input[i]->getNodeName()] = inputIdx[i];
            } else {
                inputIdx[i] = this->xCounter;
                this->tanNameMxIndex[input[i]->getNodeName()] = inputIdx[i];
                this->xCounter++;
            }
        }
    }

    int outputIdx = this->xCounter;
    this->xCounter++;
    this->tanNameMxIndex[result->getNodeName()] = outputIdx;
    MILPcons::bNXorC3(this->modelPath, inputIdx, outputIdx, this->yCounter);
}

void BwMILP::Uint1AddMinusGenModel(vector<ThreeAddressNodePtr> &input1, vector<ThreeAddressNodePtr> &input2,
                                   const vector<ThreeAddressNodePtr> &output, int AddOrMinus) {
    this->arxFlag = true;
    std::vector<int> inputIdx1, inputIdx2;
    std::vector<std::vector<ThreeAddressNodePtr>> input = {input1, input2};
    for (auto in: input1) {
        if (this->tanNameMxIndex.count(in->getNodeName()) != 0) {
            inputIdx1.push_back(this->tanNameMxIndex.find(in->getNodeName())->second);
        } else if (!this->rtnIdxSave.empty()) {
            this->tanNameMxIndex[in->getNodeName()] = this->rtnIdxSave.front();
            inputIdx1.push_back(this->rtnIdxSave.front());
            this->rtnIdxSave.erase(this->rtnIdxSave.cbegin());
        } else {
            this->tanNameMxIndex[in->getNodeName()] = this->xCounter;
            inputIdx1.push_back(this->xCounter);
            this->xCounter++;
        }
    }

    for (auto in: input2) {
        if (this->tanNameMxIndex.count(in->getNodeName()) != 0) {
            inputIdx2.push_back(this->tanNameMxIndex.find(in->getNodeName())->second);
        } else if (!this->rtnIdxSave.empty()) {
            this->tanNameMxIndex[in->getNodeName()] = this->rtnIdxSave.front();
            inputIdx2.push_back(this->rtnIdxSave.front());
            this->rtnIdxSave.erase(this->rtnIdxSave.cbegin());
        } else {
            this->tanNameMxIndex[in->getNodeName()] = this->xCounter;
            inputIdx2.push_back(this->xCounter);
            this->xCounter++;
        }
    }

    std::vector<int> outputIdx;
    for (const auto& ele : output) {
        this->tanNameMxIndex[ele->getNodeName()] = this->xCounter;
        outputIdx.push_back(this->xCounter);
        this->xCounter++;
    }

    if (this->mode == "AS") {
        // Add
        if (AddOrMinus == 1)
            MILPcons::bAddC(this->modelPath, inputIdx1, inputIdx2, outputIdx, this->ARXineqs, dCounter, PCounter);
            // Minus
        else if (AddOrMinus == 2)
            // a - b = c   ->   a = b + c   ->   b + c = a
            MILPcons::bAddC(this->modelPath, outputIdx, inputIdx2, inputIdx1, this->ARXineqs, dCounter, PCounter);
        else
            assert(false);
    } else if (this->mode == "DC") {
        // Add
        if (AddOrMinus == 1)
            MILPcons::dAddC(this->modelPath, inputIdx1, inputIdx2, outputIdx, this->ARXineqs, dCounter, PCounter);
            // Minus
        else if (AddOrMinus == 2)
            // a - b = c   ->   a = b + c   ->   b + c = a
            MILPcons::dAddC(this->modelPath, outputIdx, inputIdx2, inputIdx1, this->ARXineqs, dCounter, PCounter);
        else
            assert(false);
    } else
        assert(false);
}

void BwMILP::solver() {
    clock_t startTime, endTime;
    startTime = clock();
    time_t star_time = 0, end_time;
    star_time = time(NULL);
    GRBEnv env = GRBEnv(true);

    // 设置线程数
    env.set(GRB_IntParam_Threads, this->gurobiThreads);
    //env.set(GRB_IntParam_MIPFocus, 3);
    env.start();
    GRBModel model = GRBModel(env, modelPath);
    model.set(GRB_IntParam_MIPFocus, 2);
    // timer
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

    // Obtaining the specific values of variables
    // all variable and results mapping
    map<std::string, int> dcAus;
    GRBVar *a = model.getVars();
    int numvars = model.get(GRB_IntAttr_NumVars);
    for (int i = 0; i < numvars; ++i) {
        f << a[i].get(GRB_StringAttr_VarName) << " = " << a[i].get(GRB_DoubleAttr_X) << "\n";
        dcAus[a[i].get(GRB_StringAttr_VarName)] = int(a[i].get(GRB_DoubleAttr_X));
    }
    f.close();

    // save differential characteristic
    std::ofstream dcs(this->dcPath, std::ios::trunc);
    if (!dcs){
        std::cout << "Wrong file path ! " << std::endl;
    } else {
        for (int j = 0; j < differentialCharacteristic.size(); ++j) {
            if (j%2 == 0)
                dcs << j/2 << "-th input difference\n";
            else
                dcs << (j-1)/2 << "-th output difference\n";
            for (int k = 0; k < differentialCharacteristic[j].size(); ++k) {
                dcs << dcAus["x"+std::to_string(differentialCharacteristic[j][k])];
                if ((k+1)%8 == 0)
                    dcs << " ";
                if (k == differentialCharacteristic[j].size() - 1)
                    dcs << "\n";
            }
        }
    }
    dcs.close();

    std::cout << "***********************************" << std::endl;
    std::cout << "      Obj is  : " << result << std::endl;
    std::cout << "***********************************" << std::endl;
    finalResults.push_back(result);
    finalClockTime.push_back(clockTime);
    finalTimeTime.push_back(timeTime);
}

