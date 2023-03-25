//
// Created by Septi on 7/5/2022.
//

#include "Inliner.h"

/**
 * 对program中涉及到的gadgets进行inline操作，并将结果都存放在nameToInlinedProcedure中
 */
void Inliner::inlineProcedures() {
    // 如果是简单的simple gadget，则直接存储
    for(auto ele : procedures) {
        if(ele->isSimple1()) {
            // 如果是简单的gadget
            nameToInlinedProcedure[ele->getName()] = ele;
        } else {
            // 如果不是简单的gadget
            vector<ThreeAddressNodePtr> functionCall;
            map<ThreeAddressNodePtr, vector<ActualPara>> mapCallToArguments;
            map<ThreeAddressNodePtr, vector<ThreeAddressNodePtr>> mapCallToReturnValues;
            map<ThreeAddressNodePtr, set<ThreeAddressNodePtr>> mapFuncCallArgumentsToCalls;
            map<ActualPara, set<ThreeAddressNodePtr>> mapInputArgumentsToCalls;

            map<ThreeAddressNodePtr, ThreeAddressNodePtr> mapReturnToInlined;

            vector<ThreeAddressNodePtr> inlinedBlock;
            vector<ThreeAddressNodePtr> inlinedReturn;

            ele->arrangeArguments(functionCall, mapCallToArguments, mapFuncCallArgumentsToCalls, mapInputArgumentsToCalls, mapCallToReturnValues);
            auto callToNodeName = ele->getCallToNodeName();

            for (int p = 0; p < functionCall.size(); p++) {

                auto callInstruction = functionCall[p];
                string nodeName = callInstruction->getNodeName();
                string::size_type pos1 = callInstruction->getNodeName().find("@");
                string calleeName = callInstruction->getNodeName().substr(0, pos1);
                ProcedureHPtr callee = nameToInlinedProcedure[calleeName];
                map<string, ThreeAddressNodePtr> saved;
                map<ThreeAddressNodePtr, ThreeAddressNodePtr> savedForFunction;
                vector<ThreeAddressNodePtr> newBlock;
                vector<ThreeAddressNodePtr> newReturn;

                // 主要还是要在这里获得实际参数，但是现在实际参数可能是之前函数调用的返回值。
                auto actualArguments = mapCallToArguments[callInstruction];
                // 这里的操作是，如果有已经被实例化的，那么直接替换掉
                for(auto& ele : actualArguments) {
                    vector<ThreeAddressNodePtr> temp;
                    for(auto ele1 : ele.first) {
                        if(mapReturnToInlined.count(ele1)!=0) {
                            temp.push_back(mapReturnToInlined[ele1]);
                        } else {
                            temp.push_back(ele1);
                        }
                    }
                    ele.first = temp;
                }

                instantitateProcedure(callee, actualArguments, callInstruction,
                                      newBlock, newReturn, saved, savedForFunction, callToNodeName);

                for(int i = 0; i < mapCallToReturnValues[callInstruction].size(); i++){
                    mapReturnToInlined[mapCallToReturnValues[callInstruction][i]] = newReturn[i];
                }
                inlinedBlock.insert(inlinedBlock.end(), newBlock.begin(), newBlock.end());
                inlinedReturn = newReturn;

            }

            /*for(auto ele : inlinedBlock) {
                cout << ele->getNodeName() << " = " << ele->prettyPrint5() << endl;
            }*/

//            make_shared<ProcedureH>()

            auto newProc = ProcedureHPtr(new ProcedureH(ele->getName(), ele->getParameters(),
                                                        inlinedBlock, inlinedReturn, ele->getNameToProc()));

            nameToInlinedProcedure[ele->getName()] = newProc;

        }
        //cout << ele->getName() << ": " << ele->isSimple1() << endl;

    }

}


/**
 * 将函数调用进行实例化，返回实例化后的结果
 * @param proc
 * @param actualArguments
 * @param callInstruction
 * @param newBlock
 * @param saved
 * @param savedForFunction
 * @param callToNodeName
 */
void Inliner::instantitateProcedure(const ProcedureHPtr proc, const vector<ActualPara>& actualArguments, const ThreeAddressNodePtr& callInstruction,
                                    vector<ThreeAddressNodePtr>& newBlock, vector<ThreeAddressNodePtr>& newReturn,
                                    map<string, ThreeAddressNodePtr>& saved, map<ThreeAddressNodePtr, ThreeAddressNodePtr>& savedForFunction, const map<ThreeAddressNodePtr, string>& callToNodeName) {
    string::size_type pos1 = callInstruction->getNodeName().find('@');
    string path = callInstruction->getNodeName().substr(pos1, callInstruction->getNodeName().size());
    map<ThreeAddressNodePtr, ThreeAddressNodePtr> formalToActual;
    for(int i = 0; i < proc->getParameters().size(); i++) {
        ActualPara temp = actualArguments[i];
        auto parameters = proc->getParameters()[i];
        for(int j = 0; j < parameters.size(); j++) {
            formalToActual[parameters[j]] = temp.first[j];
        }
    }


    // 实例化returns，使用具体的computation
    set<ThreeAddressNodePtr> backup;
    for(auto ele: nameToInlinedProcedure[proc->getName()]->getBlock()) {
        if(ele->getNodeName() != "push" && saved.count(ele->getNodeName()) != 0)
            newBlock.push_back(saved[ele->getNodeName()]);
        else {
            ThreeAddressNodePtr temp = instantitateNode(proc, callToNodeName.at(callInstruction), ele, formalToActual, saved, savedForFunction,
                                                        path, backup);
            newBlock.push_back(temp);
        }
    }
    for(auto ele: nameToInlinedProcedure[proc->getName()]->getReturns()) {
        if(ele->getNodeName() != "push" && saved.count(ele->getNodeName()) != 0)
            newReturn.push_back(saved[ele->getNodeName()]);
        else {
            ThreeAddressNodePtr temp = instantitateNode(proc, callToNodeName.at(callInstruction), ele, formalToActual, saved, savedForFunction,
                                                        path, backup);
            newReturn.push_back(temp);
        }
    }

}

/**
 * 将结点进行实例化，用实际参数代替形式参数
 * @param proc
 * @param basename
 * @param node
 * @param formalToActual
 * @param saved
 * @param savedForFunction
 * @param path
 * @param randomBackup
 * @return
 */

ThreeAddressNodePtr Inliner::instantitateNode(ProcedureHPtr proc, string basename, ThreeAddressNodePtr node,
                                              map<ThreeAddressNodePtr, ThreeAddressNodePtr>& formalToActual, map<string, ThreeAddressNodePtr>& saved, map<ThreeAddressNodePtr, ThreeAddressNodePtr >& savedForFunction, string path, set<ThreeAddressNodePtr>& randomBackup) {
    if(!node)
        return nullptr;

    if(formalToActual.count(node) != 0) {
        // 如果直接是参数，直接返回结果
//        if(formalToActual[node]->getOp() == ASTNode::Operator::CALL)
//            cout << "hello" << endl;
//        if(formalToActual[node]->getNodeType() == NodeType::FUNCTION)
//            cout << "hello" << endl;
        saved[node->getNodeName()] = formalToActual[node];
        return formalToActual[node];
    } else if(node->getNodeName() != "push" && saved.count(node->getNodeName()) != 0) {
        // 如果相同名称的节点已经在在save中了，那么就直接返回那个节点
        return saved[node->getNodeName()];
    } else if(node->getNodeName() == "push" && saved.count(node->getNodeName()) != 0 &&
              saved[node->getNodeName()]->getLhs()->getNodeName() == node->getLhs()->getNodeName())  {
        return saved[node->getNodeName()];
    } else if(savedForFunction.count(node) != 0) {
        return savedForFunction[node];
    }
    // 新增一个touint处理后的情况
    /*else if (node->getOp() == ASTNode::BOXINDEX) {
        ThreeAddressNodePtr left = instantitateNode(proc, basename, node->getLhs(), formalToActual, saved, savedForFunction, path, randomBackup);

    }*/
    else {
        // for debug
        /*if (node->getNodeName() == "leftover_2one")
            std::cout << "stop here " << std::endl;*/

        // 建立新的节点，并实例化，先实例化左边，再实例化右边。并且将新节点的名称以及对应的新节点存下来
        ThreeAddressNodePtr left = instantitateNode(proc, basename, node->getLhs(), formalToActual, saved, savedForFunction, path, randomBackup);
        ThreeAddressNodePtr right = instantitateNode(proc, basename, node->getRhs(), formalToActual, saved, savedForFunction, path, randomBackup);

        ThreeAddressNodePtr newNode = make_shared<ThreeAddressNode>(node->getNodeName() + path + "(" + basename + ")", left, right, node->getOp(), node->getNodeType());
        /*if(newNode->getNodeType() == NodeType::RANDOM){
            randomBackup.insert(node);
        }*/
        if(left && left->getNodeType() == NodeType::FUNCTION)
            newNode->setIndexCall(node->getIndexCall());
        if(left)
            left->addParents(newNode);
        if(right)
            right->addParents(newNode);

        if(node->getNodeName() != "push" && saved.count(node->getNodeName()) != 0)
            //assert(false);
        if(node->getNodeType() == NodeType::FUNCTION) {
            // function 和其他普通节点分开来考虑了，目的是为了考虑到AddRoundKey这种在recompuate的时候会出问题。
            savedForFunction[node] = newNode;
        } else {
            saved[node->getNodeName()] = newNode;
        }

        return newNode;
    }
}