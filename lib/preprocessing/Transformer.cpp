//
// Created by Septi on 6/27/2022.
//

#include "Transformer.h"
#include "ThreeAddressNode.h"
#include "Interpreter.h"


ProcedureHPtr Transformer::transform(ProcValuePtr procValuePtr) {
    vector<vector<ThreeAddressNodePtr>> parameters;
    vector<ThreeAddressNodePtr> block;
    vector<ThreeAddressNodePtr> returns;

    bool isSimple = true;

    map<ValuePtr, ThreeAddressNodePtr> saved;
    map<string, int> nameCount;

    // 进行参数的转换
    for(auto ele : procValuePtr->getParameters()) {
        if(ArrayValuePtr arrayValuePtr = dynamic_pointer_cast<ArrayValue>(ele)) {
            vector<ThreeAddressNodePtr> paras;
            for(auto ele1 : arrayValuePtr->getArrayValue()) {
                ThreeAddressNodePtr para(new ThreeAddressNode(ele1->getName(), nullptr, nullptr, ASTNode::Operator::NULLOP, NodeType::PARAMETER));
                paras.push_back(para);
                saved[ele1] = para;
            }
            parameters.push_back(paras);
        }
        else {
            vector<ThreeAddressNodePtr> paras;
            ThreeAddressNodePtr para(new ThreeAddressNode(ele->getName(), nullptr, nullptr, ASTNode::Operator::NULLOP, NodeType::PARAMETER));
            paras.push_back(para);
            saved[ele] = para;
            parameters.push_back(paras);
        }
    }

    if(procValuePtr->getName() == "preshare") {
        for(auto ele : procValuePtr->getProcedurePtr()->getBlock()) {
            /*if(RandomValuePtr randomValuePtr = dynamic_pointer_cast<RandomValue>(ele)) {
                if(saved.count(randomValuePtr) == 0)
                    block.push_back(transformRandom(randomValuePtr, saved, nameCount));
            }*/
            //else
                if(InternalBinValuePtr internalBinValuePtr = dynamic_pointer_cast<InternalBinValue>(ele)) {
                block.push_back(transformInternalBinForShare(internalBinValuePtr, saved, nameCount));
            }
        }
    }
    else {

        // 进行block的转换
        for (const auto& ele : procValuePtr->getProcedurePtr()->getBlock()) {
            // 这里我为了分析value转化为三地址形式的过程，我需要根据具体的例子来观察具体的转换过程，
            // 此外，因为转换过程还有一些包含symbol的情况会丢失地址信息(存储在左孩子或者右孩子中的信息为空)，
            // 因此，我需要根据这种情况来观察特定的过程，
            // 比如，被测试表达式中 “int sbox_out = s[sbox_in];” 的转换过程
            /*if (ele->getName().find("rtn") != ele->getName().npos)
                std::cout << "i am here !" << std::endl;*/

            if (InternalBinValuePtr internalBinValuePtr = dynamic_pointer_cast<InternalBinValue>(ele)) {
                block.push_back(transformInternalBin(internalBinValuePtr, saved, nameCount));
            }
            else if (InternalUnValuePtr internalUnValue = dynamic_pointer_cast<InternalUnValue>(ele)) {
                /*if (internalUnValue->getOp() == ASTNode::Operator::TOUINT) {
                    vector<ThreeAddressNodePtr> res = transformTouint(internalUnValue, saved, nameCount);
                    block.insert(block.end(), res.begin(), res.end());
                }
                else {
                    block.push_back(transformInternalUn(internalUnValue, saved, nameCount));
                }*/
                block.push_back(transformInternalUn(internalUnValue, saved, nameCount));
            }
            else if (ArrayValuePtr arrayValuePtr = dynamic_pointer_cast<ArrayValue>(ele)) {
                vector<ThreeAddressNodePtr> res = transformArrayValue(arrayValuePtr, saved, nameCount, isSimple);
                block.insert(block.end(), res.begin(), res.end());
            }
            /*else if (RandomValuePtr randomValuePtr = dynamic_pointer_cast<RandomValue>(ele)) {
                block.push_back(transformRandom(randomValuePtr, saved, nameCount));
            }*/
                // 这里新增一个 ArrayValueIndex 到 ThreeAddressNodePtr 的转换
            else if (shared_ptr<ArrayValueIndex> arrayValueIndex = dynamic_pointer_cast<ArrayValueIndex>(ele)) {
                block.push_back(transformArrayValueIndex(*arrayValueIndex, saved, nameCount, isSimple));
            }
        }
    }
    // 进行return的转换
    if(ArrayValuePtr arrayValuePtr = dynamic_pointer_cast<ArrayValue>(procValuePtr->getProcedurePtr()->getReturns())) {
        for(auto ele1 : arrayValuePtr->getArrayValue()) {
            if(saved.count(ele1) != 0) {
                returns.push_back(saved[ele1]);
            } else {
                if (InternalBinValuePtr internalBinValuePtr = dynamic_pointer_cast<InternalBinValue>(ele1)) {
                    returns.push_back(transformInternalBin(internalBinValuePtr, saved, nameCount));
                } else if (InternalUnValuePtr internalUnValue = dynamic_pointer_cast<InternalUnValue>(ele1)) {
                    returns.push_back(transformInternalUn(internalUnValue, saved, nameCount));
                } else if(ConcreteNumValuePtr concreteNumValue = dynamic_pointer_cast<ConcreteNumValue>(ele1)){
                    auto res = make_shared<ThreeAddressNode>(to_string(concreteNumValue->getNumer()), nullptr, nullptr,
                                                             ASTNode::Operator::NULLOP, NodeType::CONSTANT);
                    returns.push_back(res);
                    saved[ele1] = res;
                } else {
                    assert(false);
                }
            }
        }
    } else {
        returns.push_back(saved[procValuePtr->getProcedurePtr()->getReturns()]);
    }

    map<string, ProcedureHPtr> nameToProc;
    for(auto ele : procedureHs) {
        nameToProc[ele->getName()] = ele;
    }

    auto newProc = ProcedureHPtr(new ProcedureH(procValuePtr->getName(), parameters, block, returns, nameToProc));
    newProc->setIsSimple(isSimple);
    return newProc;
}

ThreeAddressNodePtr Transformer::transformInternalBinForShare(InternalBinValuePtr internalBinValuePtr, map<ValuePtr, ThreeAddressNodePtr>& saved, map<string, int>& nameCount) {
    ThreeAddressNodePtr left = nullptr;
    ThreeAddressNodePtr right = nullptr;
    string name = "";
    if(internalBinValuePtr->getName() == "")
        name = "t";
    else
        name = internalBinValuePtr->getName();

    /*if(internalBinValuePtr->getLeft()->getValueType() == ValueType::VTRandomVable &&
       saved.count(internalBinValuePtr->getLeft()) == 0) {
        string count = getCount(internalBinValuePtr->getLeft()->getName(), nameCount);
        left = make_shared<ThreeAddressNode>(internalBinValuePtr->getLeft()->getName() + count, nullptr, nullptr, ASTNode::Operator::NULLOP, NodeType::RANDOM);
        saved[internalBinValuePtr->getLeft()] = left;
    } else*/
    if(ParameterValuePtr parameterValuePtr = dynamic_pointer_cast<ParameterValue>(internalBinValuePtr->getLeft())) {
        if(saved.count(parameterValuePtr) == 0) {
            string count = getCount(internalBinValuePtr->getLeft()->getName(), nameCount);
            left = make_shared<ThreeAddressNode>(internalBinValuePtr->getLeft()->getName() + count, nullptr, nullptr, ASTNode::Operator::NULLOP, NodeType::PARAMETER);
            saved[internalBinValuePtr->getLeft()] = left;
        } else {
            left = saved[parameterValuePtr];
        }
    }
    else if(InternalBinValuePtr internalBinValuePtr1 = dynamic_pointer_cast<InternalBinValue>(internalBinValuePtr->getLeft())) {
        if(saved.count(internalBinValuePtr1) == 0) {
            left = transformInternalBinForShare(internalBinValuePtr1, saved, nameCount);
        } else {
            left = saved[internalBinValuePtr->getLeft()];
        }
    }

    if(saved.count(internalBinValuePtr->getRight()) != 0) {
        right = saved[internalBinValuePtr->getRight()];
    }
    /*else if(internalBinValuePtr->getRight()->getValueType() == ValueType::VTRandomVable &&
              saved.count(internalBinValuePtr->getRight()) == 0) {
        string count = getCount(internalBinValuePtr->getRight()->getName(), nameCount);
        right = make_shared<ThreeAddressNode>(internalBinValuePtr->getRight()->getName() + count, nullptr, nullptr, ASTNode::Operator::NULLOP, NodeType::RANDOM);
        saved[internalBinValuePtr->getRight()] = right;
    }*/
    else if(InternalBinValuePtr internalBinValuePtr1 = dynamic_pointer_cast<InternalBinValue>(internalBinValuePtr->getRight())) {
        if(saved.count(internalBinValuePtr1) == 0) {
            right = transformInternalBinForShare(internalBinValuePtr1, saved, nameCount);
        } else {
            right = saved[internalBinValuePtr->getRight()];
        }
    }

    string count = getCount(name, nameCount);
    // ThreeAddressNodePtr para(new ThreeAddressNode(name + count, left, right, internalBinValuePtr->getOp(), NodeType::INTERNAL));
    // 这里先把NodeTYpe改了，为了先编译成功再说
    ThreeAddressNodePtr para(new ThreeAddressNode(name + count, left, right, internalBinValuePtr->getOp(), NodeType::CONSTANT));
    left->addParents(para);
    right->addParents(para);
    saved[internalBinValuePtr] = para;

    return para;

}

ThreeAddressNodePtr Transformer::transformInternalBin(InternalBinValuePtr internalBinValuePtr, map<ValuePtr, ThreeAddressNodePtr>& saved, map<string, int>& nameCount) {
    ThreeAddressNodePtr left = nullptr;
    ThreeAddressNodePtr right = nullptr;
    string name = "";
    if(internalBinValuePtr->getName() == "")
        name = "t";
    else
        name = internalBinValuePtr->getName();

    if(ConcreteNumValuePtr concreteNumValuePtr = dynamic_pointer_cast<ConcreteNumValue>(internalBinValuePtr->getLeft())){
        if(saved.count(internalBinValuePtr->getLeft()) == 0) {
            left = make_shared<ThreeAddressNode>(to_string(concreteNumValuePtr->getNumer()), nullptr, nullptr,
                                                 ASTNode::Operator::NULLOP, NodeType::CONSTANT);
            saved[internalBinValuePtr->getLeft()] = left;
        } else {
            left = saved[internalBinValuePtr->getLeft()];
        }
    }
    // 这里因为我们也考虑数组的index为symbol的情况，所以左右孩子节点都有可能包含是包含symbol的arrayvalueindex的类型
    // 所以这里要对左右孩子节点都进行这样的判断，
    else if (shared_ptr<ArrayValueIndex> arrayValueIndex = dynamic_pointer_cast<ArrayValueIndex>(internalBinValuePtr->getLeft())) {
        bool temp_simple = true;
        left = transformArrayValueIndex(*arrayValueIndex, saved, nameCount, temp_simple);
    }
    // 当left是BoxValue时，对应的internalBin为BOXOP，如： uint4 sbox_out = sbox<sbox_in>;
    else if (BoxValuePtr boxValuePtr = dynamic_pointer_cast<BoxValue>(internalBinValuePtr->getLeft())) {
        bool temp_simple = true;
        left = transformBoxValue(boxValuePtr, saved, temp_simple);
    }
    // 当左孩子也为internalbin时，递归调用
    else if (InternalBinValuePtr internalBinValuePtr1 = dynamic_pointer_cast<InternalBinValue>(internalBinValuePtr->getLeft())) {
        //left = transformInternalBin(internalBinValuePtr1, saved, nameCount);
        if(saved.count(internalBinValuePtr1) == 0) {
            left = transformInternalBin(internalBinValuePtr1, saved, nameCount);
        } else {
            left = saved[internalBinValuePtr1];
        }
    }
    else {
        left = saved[internalBinValuePtr->getLeft()];
        if (left == nullptr) {
            if (internalBinValuePtr->getLeft()->toString().find("[SYMBOL]") != string::npos) {
                // 此时left的值是不可计算的，因为可能与输入相关，那么就直接返回表达式表示的形式。
                // 所以此时不能使用存储在saved中的具体数值，而是需要以表达式的方式存储。
                ThreeAddressNodePtr temp_left(new ThreeAddressNode(internalBinValuePtr->getLeft()->getName(),
                                                                   nullptr, nullptr, ASTNode::Operator::NULLOP, NodeType::UNKNOWN));
                left = temp_left;
            }
/*
            if (InternalBinValuePtr left_internalBinValuePtr = dynamic_pointer_cast<InternalBinValue>(
                    internalBinValuePtr->getLeft())) {
                left = transformInternalBin(left_internalBinValuePtr, saved, nameCount);
            } else if (ArrayValuePtr left_arrayValuePtr = dynamic_pointer_cast<ArrayValue>(
                    internalBinValuePtr->getLeft())) {
                bool isSimple = true;
                vector<ThreeAddressNodePtr> temp = transformArrayValue(left_arrayValuePtr, saved, nameCount, isSimple);
                left = temp.at(0);
            }
*/
        }
        assert(left);
    }

    /*if(internalBinValuePtr->getRight()->getValueType() == ValueType::VTRandomVable &&
       saved.count(internalBinValuePtr->getRight()) == 0) {
        string count = getCount(internalBinValuePtr->getRight()->getName(), nameCount);
        right = make_shared<ThreeAddressNode>(internalBinValuePtr->getRight()->getName() + count, nullptr, nullptr, ASTNode::Operator::NULLOP, NodeType::RANDOM);
        saved[internalBinValuePtr->getRight()] = right;
    } else*/
    if(ConcreteNumValuePtr concreteNumValuePtr = dynamic_pointer_cast<ConcreteNumValue>(internalBinValuePtr->getRight())){
        if(saved.count(internalBinValuePtr->getRight()) == 0) {
            right = make_shared<ThreeAddressNode>(to_string(concreteNumValuePtr->getNumer()), nullptr, nullptr,
                                                  ASTNode::Operator::NULLOP, NodeType::CONSTANT);
            saved[internalBinValuePtr->getRight()] = right;
        } else {
            right = saved[internalBinValuePtr->getRight()];
        }
    }
    // 同样的，我们也增加对right的判断，看是否其是包含symbol的数组index访问
    else if (shared_ptr<ArrayValueIndex> arrayValueIndex = dynamic_pointer_cast<ArrayValueIndex>(internalBinValuePtr->getRight())) {
        bool temp_simple = true;
        right = transformArrayValueIndex(*arrayValueIndex, saved, nameCount, temp_simple);
    }
    // 当right为arrayValue类型时，说明是对arrayValue整体进行的操作，如 sbox substitution
    // 此时，需要将arrayValue转化成internalBinValuePtr模式的三地址
    else if (ArrayValuePtr arrayValuePtr = dynamic_pointer_cast<ArrayValue>(internalBinValuePtr->getRight())) {
        bool temp_simple = true;
        right = transformArrayValue2oneTAN(arrayValuePtr, saved, nameCount, temp_simple);
    }
    else if (InternalBinValuePtr internalBinValuePtr1 = dynamic_pointer_cast<InternalBinValue>(internalBinValuePtr->getRight())) {
        // right = transformInternalBin(internalBinValuePtr1, saved, nameCount);
        if(saved.count(internalBinValuePtr1) == 0) {
            right = transformInternalBin(internalBinValuePtr1, saved, nameCount);
        } else {
            right = saved[internalBinValuePtr1];
        }
    }
    else {
        right = saved[internalBinValuePtr->getRight()];
        assert(right);
    }

    // 这里我们改一下，当name == “t” 时，才在后面加 nameCount
    /*string count;
    if (name == "t")
        count = getCount(name, nameCount);
    else
        count = "";*/

    // 7.06.2022更改：无论name等于何值，都需要在后面加上count，防止三地址实例名称重复
    // 注意：在实际代码中我们可能对同一个变量进行多次赋值，此时在进行转换时，
    // 直接在后面添加一个counter会产生变量标识符重复覆盖的情况，如 p_out[0] = sbox<sbox_in>[0]; p_out[0] = p_out[0] ^ r;
    // 此时两次出现的 p_out[0] 的变量标识符分别为 p_out0 和 p_out01, 貌似没有问题，但是当 p_out[i] 中 i 取 5 呢？
    // 就有两个变量标识符 p_out5 和 p_out51， 会覆盖掉原来名为 p_out51 的变量，所以我们需要设计一种可以避免重复赋值情况下出现重复覆盖的情况

    // 这里我们检查本节点的name是否和其中一个孩子节点的name相同，如果相同，说明是对一个变量的重复赋值，用 "n" + name标记避免重复
    string count;
    bool duplicateAssignmentFlag = false;
    if (name == left->getNodeName() or name == right->getNodeName()) {
        count = "";
        duplicateAssignmentFlag = true;
    }
    else
        count = getCount(name, nameCount);
    //ThreeAddressNodePtr para(new ThreeAddressNode(name + count, left, right, internalBinValuePtr->getOp(), NodeType::INT));

    // 2022.7.1.21.46
    // 新增一个根据varType选择NodeType的函数，从而NodeType根据VarType来决定，分析时可以根据uints的s来决定
    NodeType nodeType = getNodeType(internalBinValuePtr->getVarType());
    ThreeAddressNodePtr para(new ThreeAddressNode(name + count, left, right, internalBinValuePtr->getOp(), nodeType));
    left->addParents(para);
    right->addParents(para);
    saved[internalBinValuePtr] = para;
    return para;
}


// 暂时先不用，再考虑考虑
vector<ThreeAddressNodePtr>
Transformer::transformInternalBinVec(InternalBinValuePtr internalBinValuePtr, map<ValuePtr, ThreeAddressNodePtr> &saved,
                        map<string, int> &nameCount) {
    ThreeAddressNodePtr left = nullptr;
    ThreeAddressNodePtr right = nullptr;
    string name = "";
    if(internalBinValuePtr->getName() == "")
        name = "t";
    else
        name = internalBinValuePtr->getName();

    /*if(internalBinValuePtr->getLeft()->getValueType() == ValueType::VTRandomVable &&
       saved.count(internalBinValuePtr->getLeft()) == 0) {
        string count = getCount(internalBinValuePtr->getLeft()->getName(), nameCount);
        left = make_shared<ThreeAddressNode>(internalBinValuePtr->getLeft()->getName() + count, nullptr, nullptr, ASTNode::Operator::NULLOP, NodeType::RANDOM);
        saved[internalBinValuePtr->getLeft()] = left;
    } else*/
    if(ConcreteNumValuePtr concreteNumValuePtr = dynamic_pointer_cast<ConcreteNumValue>(internalBinValuePtr->getLeft())){
        if(saved.count(internalBinValuePtr->getLeft()) == 0) {
            left = make_shared<ThreeAddressNode>(to_string(concreteNumValuePtr->getNumer()), nullptr, nullptr,
                                                 ASTNode::Operator::NULLOP, NodeType::CONSTANT);
            saved[internalBinValuePtr->getLeft()] = left;
        } else {
            left = saved[internalBinValuePtr->getLeft()];
        }
    }
        // 这里因为我们也考虑数组的index为symbol的情况，所以左右孩子节点都有可能包含是包含symbol的arrayvalueindex的类型
        // 所以这里要对左右孩子节点都进行这样的判断，
    else if (shared_ptr<ArrayValueIndex> arrayValueIndex = dynamic_pointer_cast<ArrayValueIndex>(internalBinValuePtr->getLeft())) {
        bool temp_simple = true;
        left = transformArrayValueIndex(*arrayValueIndex, saved, nameCount, temp_simple);
    }
        // 当left是BoxValue时，对应的internalBin为BOXOP，如： uint4 sbox_out = sbox<sbox_in>;
    else if (BoxValuePtr boxValuePtr = dynamic_pointer_cast<BoxValue>(internalBinValuePtr->getLeft())) {
        bool temp_simple = true;
        left = transformBoxValue(boxValuePtr, saved, temp_simple);
    }
    else {
        left = saved[internalBinValuePtr->getLeft()];
        if (left == nullptr) {
            // for debug
            //std::cout << "debug : " << internalBinValuePtr->getLeft()->toString() << std::endl;
            //std::cout << "left name : " << internalBinValuePtr->getLeft()->getName() << std::endl;
            if (internalBinValuePtr->getLeft()->toString().find("[SYMBOL]") != string::npos) {
                // 此时left的值是不可计算的，因为可能与输入相关，那么就直接返回表达式表示的形式。
                // 所以此时不能使用存储在saved中的具体数值，而是需要以表达式的方式存储。
                ThreeAddressNodePtr temp_left(new ThreeAddressNode(internalBinValuePtr->getLeft()->getName(),
                                                                   nullptr, nullptr, ASTNode::Operator::NULLOP, NodeType::UNKNOWN));
                left = temp_left;
            }
/*
            if (InternalBinValuePtr left_internalBinValuePtr = dynamic_pointer_cast<InternalBinValue>(
                    internalBinValuePtr->getLeft())) {
                left = transformInternalBin(left_internalBinValuePtr, saved, nameCount);
            } else if (ArrayValuePtr left_arrayValuePtr = dynamic_pointer_cast<ArrayValue>(
                    internalBinValuePtr->getLeft())) {
                bool isSimple = true;
                vector<ThreeAddressNodePtr> temp = transformArrayValue(left_arrayValuePtr, saved, nameCount, isSimple);
                left = temp.at(0);
            }
*/
        }
        assert(left);
    }

    /*if(internalBinValuePtr->getRight()->getValueType() == ValueType::VTRandomVable &&
       saved.count(internalBinValuePtr->getRight()) == 0) {
        string count = getCount(internalBinValuePtr->getRight()->getName(), nameCount);
        right = make_shared<ThreeAddressNode>(internalBinValuePtr->getRight()->getName() + count, nullptr, nullptr, ASTNode::Operator::NULLOP, NodeType::RANDOM);
        saved[internalBinValuePtr->getRight()] = right;
    } else*/
    if(ConcreteNumValuePtr concreteNumValuePtr = dynamic_pointer_cast<ConcreteNumValue>(internalBinValuePtr->getRight())){
        if(saved.count(internalBinValuePtr->getRight()) == 0) {
            right = make_shared<ThreeAddressNode>(to_string(concreteNumValuePtr->getNumer()), nullptr, nullptr,
                                                  ASTNode::Operator::NULLOP, NodeType::CONSTANT);
            saved[internalBinValuePtr->getRight()] = right;
        } else {
            right = saved[internalBinValuePtr->getRight()];
        }
    }
        // 同样的，我们也增加对right的判断，看是否其是包含symbol的数组index访问
    else if (shared_ptr<ArrayValueIndex> arrayValueIndex = dynamic_pointer_cast<ArrayValueIndex>(internalBinValuePtr->getRight())) {
        bool temp_simple = true;
        right = transformArrayValueIndex(*arrayValueIndex, saved, nameCount, temp_simple);
    }
        // 当right为arrayValue类型时，说明是对arrayValue整体进行的操作，如 sbox substitution
        // 此时，需要将arrayValue转化成internalBinValuePtr模式的三地址
    else if (ArrayValuePtr arrayValuePtr = dynamic_pointer_cast<ArrayValue>(internalBinValuePtr->getRight())) {
        bool temp_simple = true;
        right = transformArrayValue2oneTAN(arrayValuePtr, saved, nameCount, temp_simple);
    }
    else {
        right = saved[internalBinValuePtr->getRight()];
        assert(right);
    }

    // 在对三地址进行处理时，目前的设计是不考虑基本类型的，默认所有的类型都统一一致，
    // 所以这里我们根据被赋值对象uints中s的size来决定返回的数目
    int rtnSize = Interpreter::getVarTypeSize(internalBinValuePtr->getVarType());
    vector<ThreeAddressNodePtr> rtn;
    for (int i = 0; i < rtnSize; ++i) {
        // 这里我们改一下，当name == “t” 时，才在后面加 nameCount
        string count;
        if (name == "t")
            count = getCount(name, nameCount);
        else
            count = "";
        ThreeAddressNodePtr index(new ThreeAddressNode(std::to_string(i), nullptr, nullptr,
                                                       ASTNode::Operator::NULLOP, NodeType::INDEX));
        ThreeAddressNodePtr para(new ThreeAddressNode(name + count, left, right, internalBinValuePtr->getOp(), NodeType::INT));
        left->addParents(para);
        right->addParents(para);

        saved[internalBinValuePtr] = para;
    }
    return rtn;
}


ThreeAddressNodePtr Transformer::transformInternalUn(InternalUnValuePtr internalUnValuePtr, map<ValuePtr, ThreeAddressNodePtr>& saved, map<string, int>& nameCount) {

    string name = "";
    if(internalUnValuePtr->getName() == "")
        name = "t";
    else
        name = internalUnValuePtr->getName();

    ThreeAddressNodePtr operation = saved.count(internalUnValuePtr->getRand()) == 0 ? nullptr : saved[internalUnValuePtr->getRand()];

    if(operation == nullptr) {
        if(ConcreteNumValuePtr concreteNumValuePtr = dynamic_pointer_cast<ConcreteNumValue>(internalUnValuePtr->getRand())) {
            operation =  make_shared<ThreeAddressNode>(to_string(concreteNumValuePtr->getNumer()), nullptr, nullptr, ASTNode::Operator::NULLOP, NodeType::CONSTANT);
            saved[internalUnValuePtr->getRand()] = operation;
        }
        // 当rand为arrayValue类型时，说明是对arrayValue整体进行的操作，如 touint
        // 此时，需要将arrayValue转化成internalBinValuePtr模式的三地址
        else if (ArrayValuePtr arrayValuePtr = dynamic_pointer_cast<ArrayValue>(internalUnValuePtr->getRand())) {
            bool temp_simple = true;
            operation = transformArrayValue2oneTAN(arrayValuePtr, saved, nameCount, temp_simple);
        }
        else {
            assert(false);
        }
    }

    string count = getCount(name, nameCount);
//    ThreeAddressNodePtr para(new ThreeAddressNode(name + count, operation, nullptr, internalUnValuePtr->getOp(), NodeType::INTERNAL));
    // 2022.7.1.21.56
    // 新增一个根据varType选择NodeType的函数，从而NodeType根据VarType来决定，分析时可以根据uints的s来决定
    NodeType nodeType = getNodeType(internalUnValuePtr->getVarType());
    ThreeAddressNodePtr para(new ThreeAddressNode(name + count, operation, nullptr, internalUnValuePtr->getOp(), nodeType));

    operation->addParents(para);

    saved[internalUnValuePtr] = para;

    return para;
}


// 2022.7.1
// 考虑了一下，专门写一个用于转换TOUINT的函数，
// 因为TOUINT的函数赋值为uints，而在对三地址进行处理时，目前的设计是不考虑基本类型的，默认所有的类型都统一一致，
// 所以这里我们根据被赋值对象uints中s的size来决定返回的数目
vector<ThreeAddressNodePtr>
Transformer::transformTouint(InternalUnValuePtr internalUnValuePtr, map<ValuePtr, ThreeAddressNodePtr> &saved,
                map<string, int> &nameCount) {

    int rtnSize = Interpreter::getVarTypeSize(internalUnValuePtr->getVarType());
    vector<ThreeAddressNodePtr> rtn;
    for (int i = 0; i < rtnSize; ++i) {
        string name = "";
        if(internalUnValuePtr->getName() == "")
            name = "t";
        else
            name = internalUnValuePtr->getName();

        ThreeAddressNodePtr operation = saved.count(internalUnValuePtr->getRand()) == 0 ? nullptr : saved[internalUnValuePtr->getRand()];

        if(operation == nullptr) {
            if(ConcreteNumValuePtr concreteNumValuePtr = dynamic_pointer_cast<ConcreteNumValue>(internalUnValuePtr->getRand())) {
                operation =  make_shared<ThreeAddressNode>(to_string(concreteNumValuePtr->getNumer()), nullptr, nullptr, ASTNode::Operator::NULLOP, NodeType::CONSTANT);
                saved[internalUnValuePtr->getRand()] = operation;
            }
                // 当rand为arrayValue类型时，说明是对arrayValue整体进行的操作，如 touint
                // 此时，需要将arrayValue转化成internalBinValuePtr模式的三地址
            else if (ArrayValuePtr arrayValuePtr = dynamic_pointer_cast<ArrayValue>(internalUnValuePtr->getRand())) {
                bool temp_simple = true;
                operation = transformArrayValue2oneTAN(arrayValuePtr, saved, nameCount, temp_simple);
            }
            else {
                assert(false);
            }
        }

        string count = getCount(name, nameCount);
//    ThreeAddressNodePtr para(new ThreeAddressNode(name + count, operation, nullptr, internalUnValuePtr->getOp(), NodeType::INTERNAL));
        ThreeAddressNodePtr index(new ThreeAddressNode(std::to_string(i), nullptr, nullptr,
                                                          ASTNode::Operator::NULLOP, NodeType::INDEX));
        ThreeAddressNodePtr para(new ThreeAddressNode(name + count, operation, index,
                                                      internalUnValuePtr->getOp(), NodeType::INT));

        operation->addParents(para);

        saved[internalUnValuePtr] = para;
        rtn.push_back(para);
    }
    return rtn;
}


vector<ThreeAddressNodePtr> Transformer::transformArrayValue(ArrayValuePtr arrayValuePtr,
                                                             map<ValuePtr, ThreeAddressNodePtr>& saved, map<string, int>& nameCount, bool& isSimple) {
    vector<ThreeAddressNodePtr> res;

    for(auto ele : arrayValuePtr->getArrayValue()) {
        if(ProcCallValueIndexPtr procCallValueIndexPtr = dynamic_pointer_cast<ProcCallValueIndex>(ele)) {
            // 搞实际参数
            isSimple = false;
            ProcCallValuePtr procCallValuePtr = dynamic_pointer_cast<ProcCallValue>(procCallValueIndexPtr->getProcCallValuePtr());
            for(int i = 0; i < procCallValuePtr->getArguments().size(); i++) {
                auto ele = procCallValuePtr->getArguments()[i];

                if(ArrayValuePtr arrayValuePtr1 = dynamic_pointer_cast<ArrayValue>(ele)) {
                    for(auto ele1 : arrayValuePtr1->getArrayValue()) {
                        // 如果被调用的对象是轮函数，则需要对每个参数的名字标记处理
                        std::string name = "push";
                        if (procCallValuePtr->getProcedurePtr()->getIsRndf()) {
                            if (i == 1)
                                name = "key_push";
                            else if (i == 2)
                                name = "plaintext_push";
                        }
                        if(ConcreteNumValuePtr concreteNumValue = dynamic_pointer_cast<ConcreteNumValue>(ele1)) {
                            auto tempres = make_shared<ThreeAddressNode>(to_string(concreteNumValue->getNumer()),
                                                                         nullptr, nullptr,
                                                                         ASTNode::Operator::NULLOP, NodeType::CONSTANT);
                            res.push_back(tempres);
                            saved[ele1] = tempres;
                            ThreeAddressNodePtr threeAddressNodePtr(
                                    new ThreeAddressNode(name, saved[ele1], nullptr, ASTNode::Operator::PUSH,
                                                         NodeType::INT));
                        }
                        else {
                            assert(saved[ele1]);
                            ThreeAddressNodePtr threeAddressNodePtr(
                                    new ThreeAddressNode(name, saved[ele1], nullptr, ASTNode::Operator::PUSH,
                                                         NodeType::INT));
                            res.push_back(threeAddressNodePtr);
                        }
                    }
                } else if(ConcreteNumValuePtr concreteNumValue = dynamic_pointer_cast<ConcreteNumValue>(ele)) {
                    auto tempres = make_shared<ThreeAddressNode>(to_string(concreteNumValue->getNumer()), nullptr, nullptr,
                                                                 ASTNode::Operator::NULLOP, NodeType::CONSTANT);
                    // res.push_back(tempres);
                    saved[ele] = tempres;
                    // 这里因为不是arrayvalue，所以在处理参数时，并没有用到push操作，但是后续进行inline时，是根据push操作来识别参数的
                    // 所以这里增加一层，改为在res中存储新增的带有push操作的threeAddressNodePtr，而非tempres
                    ThreeAddressNodePtr threeAddressNodePtr(new ThreeAddressNode
                                                                    ("push", saved[ele], nullptr, ASTNode::Operator::PUSH, NodeType::INT));
                    res.push_back(threeAddressNodePtr);
                } else {
                    assert(saved[ele]);
                    ThreeAddressNodePtr threeAddressNodePtr(new ThreeAddressNode
                                                                    ("push", saved[ele], nullptr, ASTNode::Operator::PUSH, NodeType::INT));
                    res.push_back(threeAddressNodePtr);
                }
            }
        }
        break;
    }

    for(int i = 0; i < arrayValuePtr->getArrayValue().size(); i++) {
        auto ele = arrayValuePtr->getArrayValue()[i];
        if(ProcCallValueIndexPtr procCallValueIndexPtr = dynamic_pointer_cast<ProcCallValueIndex>(ele)) {
            isSimple = false;
            ThreeAddressNodePtr func = nullptr;
            if(saved.count(procCallValueIndexPtr->getProcCallValuePtr()) == 0) {
                ThreeAddressNodePtr function(new ThreeAddressNode(procCallValueIndexPtr->getProcCallValuePtr()->getName(), nullptr, nullptr, ASTNode::Operator::NULLOP, NodeType::FUNCTION));
                saved[procCallValueIndexPtr->getProcCallValuePtr()] = function;
                func = function;
            } else {
                func = saved[procCallValueIndexPtr->getProcCallValuePtr()];
            }

            string name = "t";
            string count = getCount(name, nameCount);
            ThreeAddressNodePtr threeAddressNodePtr(new ThreeAddressNode(name + count, func, nullptr, ASTNode::Operator::CALL, NodeType::INT));
            threeAddressNodePtr->setIndexCall(i);
            res.push_back(threeAddressNodePtr);
            saved[ele] = threeAddressNodePtr;
        } else {
            // 如果只是单单的赋值操作，只是想把形式参数中的值拿过来用。
            cout << "jump array" << endl;
//            assert(saved.count(ele) != 0);
//            ThreeAddressNodePtr rhs = saved[ele];
//
//            ThreeAddressNodePtr para(new ThreeAddressNode(ele->getName() + getCount(ele->getName(), nameCount), rhs, nullptr, ASTNode::Operator::EQ , NodeType::INTERNAL));
//
//            rhs->addParents(para);
//
//            return para;
//            assert(false);
        }
    }
    return res;
}


// 本函数用于处理直接进行整体操作的ArrayValue，将arrayValue的各个元素用操作符BOXINDEX连接，转换成对应internalBinValue的对应三地址形式
ThreeAddressNodePtr Transformer::transformArrayValue2oneTAN(ArrayValuePtr arrayValuePtr, map<ValuePtr, ThreeAddressNodePtr> &saved,
                                        map<string, int> &nameCount, bool & isSimple) {
    ThreeAddressNodePtr left = nullptr;
    ThreeAddressNodePtr right = nullptr;

    // 对每个元素，按照其对象类型，生成对应类型的三地址形式
    ValuePtr ele0 = arrayValuePtr->getValueAt(0);
    if(ConcreteNumValuePtr concreteNumValuePtr = dynamic_pointer_cast<ConcreteNumValue>(ele0)){
        if(saved.count(ele0) == 0) {
            left = make_shared<ThreeAddressNode>(to_string(concreteNumValuePtr->getNumer()), nullptr, nullptr,
                                                  ASTNode::Operator::NULLOP, NodeType::CONSTANT);
            saved[ele0] = left;
        } else {
            left = saved[ele0];
        }
    }
    else if (shared_ptr<ArrayValueIndex> arrayValueIndex = dynamic_pointer_cast<ArrayValueIndex>(ele0)) {
        bool temp_simple = true;
        std::string name = arrayValueIndex->getName();
        left = transformArrayValueIndex(*arrayValueIndex, saved, nameCount, temp_simple);
        left->setNodeName(name);
    }
    else if (InternalBinValuePtr internalBinValuePtr = dynamic_pointer_cast<InternalBinValue>(ele0)) {
        // 因为transformInternalBin会更改node的名字（后面增加一个counter），
        // 所以备份初始的名字，并且后面也更新一下
        std::string name = internalBinValuePtr->getName();
        left = transformInternalBin(internalBinValuePtr, saved, nameCount);
        left->setNodeName(name);
    }
    else if (InternalUnValuePtr internalUnValue = dynamic_pointer_cast<InternalUnValue>(ele0)) {
        std::string name = internalUnValue->getName();
        left = transformInternalUn(internalUnValue, saved, nameCount);
        left->setNodeName(name);
    }
    // box的操作对象可以直接是参数
    else if (ParameterValuePtr parameterValuePtr = dynamic_pointer_cast<ParameterValue>(ele0)) {
        if(saved.count(parameterValuePtr) == 0) {
            string count = getCount(ele0->getName(), nameCount);
            left = make_shared<ThreeAddressNode>(ele0->getName() + count, nullptr, nullptr, ASTNode::Operator::NULLOP, NodeType::PARAMETER);
            saved[ele0] = left;
        } else {
            left = saved[parameterValuePtr];
        }
    }
    else {
        assert(false);
    }

    std::vector<ValuePtr> leftoverArray;
    for (int i = 1; i < arrayValuePtr->getArrayValue().size(); ++i) {
        leftoverArray.push_back(arrayValuePtr->getValueAt(i));
    }
    if (!leftoverArray.empty()) {
        ArrayValuePtr leftover = make_shared<ArrayValue>("leftover", leftoverArray);
        right = transformArrayValue2oneTAN(leftover, saved, nameCount, isSimple);

        ThreeAddressNodePtr rtn = make_shared<ThreeAddressNode>(arrayValuePtr->getName() + std::to_string(leftoverCounter), left, right,
                                                                ASTNode::Operator::BOXINDEX, NodeType::ARRAY);
        leftoverCounter++;
        return rtn;
    }
    else {
        return left;
    }
    /*for (int i = 1; i < arrayValuePtr->getArrayValue().size(); ++i) {
        ValuePtr ele = arrayValuePtr->getValueAt(i);
        // 对每个元素，按照其对象类型，生成对应类型的三地址形式
        if(ConcreteNumValuePtr concreteNumValuePtr = dynamic_pointer_cast<ConcreteNumValue>(ele)){
            if(saved.count(ele) == 0) {
                right = make_shared<ThreeAddressNode>(to_string(concreteNumValuePtr->getNumer()), nullptr, nullptr,
                                                      ASTNode::Operator::NULLOP, NodeType::CONSTANT);
                saved[ele] = right;
            } else {
                right = saved[ele];
            }
        }
        else if (shared_ptr<ArrayValueIndex> arrayValueIndex = dynamic_pointer_cast<ArrayValueIndex>(ele)) {
            bool temp_simple = true;
            right = transformArrayValueIndex(*arrayValueIndex, saved, nameCount, temp_simple);
        }
        else if (InternalBinValuePtr internalBinValuePtr = dynamic_pointer_cast<InternalBinValue>(ele)) {

        }
        else if (InternalUnValuePtr internalUnValue = dynamic_pointer_cast<InternalUnValue>(ele)) {

        }
        else {
            assert(false);
        }

    }*/
}



/*ThreeAddressNodePtr Transformer::transformRandom(RandomValuePtr randomValuePtr, map<ValuePtr, ThreeAddressNodePtr>& saved,
                                                 map<string, int> & nameCount) {
    ThreeAddressNodePtr random = nullptr;
    if(saved.count(randomValuePtr) != 0)
        return saved[randomValuePtr];
    string count = getCount(randomValuePtr->getName(), nameCount);
    random = make_shared<ThreeAddressNode>(randomValuePtr->getName() + count, nullptr, nullptr, ASTNode::Operator::NULLOP, NodeType::RANDOM);
    saved[randomValuePtr] = random;
    return random;
}*/

void Transformer::transformProcedures() {
    int sboxFuncIdx = 0; // There may be multiple functions for sboxes
    for(auto ele : procedures) {
        if (ele->getProcedurePtr()->getIsFn()) {
            ele->setName("main");
        }
        //
        if (ele->getProcedurePtr()->getIsSboxf()) {
            ele->setName("sboxFunc_" + std::to_string(sboxFuncIdx));
            sboxFuncIdx++;
        }
        ProcedureHPtr temp = transform(ele);

        nameToProc[ele->getName()] = temp;
        procedureHs.push_back(temp);
    }
}
string Transformer::getCount(string name, map<string, int>& nameCount){
    if(nameCount.count(name) == 0) {
        nameCount[name] = 0;
        return "";
    } else {
        nameCount[name]++;
        //return to_string(nameCount[name]);
        // 前面增加一个"_"作以对有些情况下，变量名会重复的区分
        return "_" + to_string(nameCount[name]);
    }
}

const vector<ProcedureHPtr> &Transformer::getProcedureHs() const {
    return procedureHs;
}

ThreeAddressNodePtr Transformer::transformArrayValueIndex(ArrayValueIndex arrayValueIndex, map<ValuePtr, ThreeAddressNodePtr> &saved,
                                                          map<string, int> &nameCount, bool &isSimple) {
    isSimple = false;
    ThreeAddressNodePtr left = nullptr;
    ThreeAddressNodePtr right = nullptr;
    //这里ArrayValueIndex转换成三地址形式，左孩子为数组名，右孩子为表达式，即寻找index的表达式，同时本节点的名字为t
    string name = "t";
    /*if(arrayValueIndex.getName() == "")
        name = "t";
    else
        name = arrayValueIndex.getName();*/
    string count = getCount(name, nameCount);

    // 左孩子节点存储对应的数组的三地址形式
    ThreeAddressNodePtr temp_left(new ThreeAddressNode(arrayValueIndex.getArrayValuePtr()->getName(), nullptr,
                                                       nullptr, ASTNode::Operator::NULLOP, NodeType::ARRAY));

    left = temp_left;
    saved[arrayValueIndex.getArrayValuePtr()] = left;
    // 右孩子节点存储访问index的对应表达式的三地址形式
    if (InternalBinValuePtr internalBinValuePtr = dynamic_pointer_cast<InternalBinValue>(arrayValueIndex.getSymbolIndex())) {
        right = transformInternalBin(internalBinValuePtr, saved, nameCount);
    }
    // 可能直接将参数作为symbol index
    else if (ParameterValuePtr parameterValuePtr = dynamic_pointer_cast<ParameterValue>(arrayValueIndex.getSymbolIndex())) {
        if(saved.count(parameterValuePtr) == 0) {
            string count = getCount(arrayValueIndex.getSymbolIndex()->getName(), nameCount);
            right = make_shared<ThreeAddressNode>(arrayValueIndex.getSymbolIndex()->getName() + count, nullptr, nullptr, ASTNode::Operator::NULLOP, NodeType::PARAMETER);
            saved[arrayValueIndex.getSymbolIndex()] = right;
        } else {
            right = saved[parameterValuePtr];
        }
    }
    else {
        assert(false);
    }
    right->setNodeName("SYMBOL");
    // ThreeAddressNodePtr temp_right(new ThreeAddressNodePtr("SYMBOL", saved[arrayValueIndex.getSymbolIndex()]));
    ThreeAddressNodePtr rtn(new ThreeAddressNode(name + count, left, right, ASTNode::Operator::INDEX, NodeType::CONSTANT));
    return rtn;
}

ThreeAddressNodePtr Transformer::transformBoxValue(BoxValuePtr boxValuePtr, map<ValuePtr, ThreeAddressNodePtr> &saved,
                                                   bool &isSimple) {
    isSimple = false;
    ThreeAddressNodePtr left = nullptr;
    ThreeAddressNodePtr right = nullptr;
    //这里BoxValue转换成三地址形式，左右孩子均为空，Box的具体值在ASTNode时另外存储，后续建模过程中单独处理，
    // 转换后的三地址形式用于索引查找对应Box的具体取值

    // 这里用boxType和box标识符连接作为标识符，可以根据boxType来判断对应的操作选择
    string name = boxValuePtr->getBoxType() + boxValuePtr->getName();
    //string name = boxValuePtr->getName(); // 这里改成直接使用sbox的标识符，因为在allBox中也是直接将标识符和sbox的内存按对存储的

    ThreeAddressNodePtr rtn(new ThreeAddressNode(name, nullptr,
                                                       nullptr, ASTNode::Operator::NULLOP, NodeType::BOX));
    saved[boxValuePtr] = rtn;
    return rtn;
}

NodeType Transformer::getNodeType(ASTNode::Type type) {
    if (type == ASTNode::uint) {
        return UINT;
    }
    else if (type == ASTNode::uint1) {
        return UINT1;
    }
    else if (type == ASTNode::uint4) {
        return UINT4;
    }
    else if (type == ASTNode::uint6) {
        return UINT6;
    }
    else if (type == ASTNode::uint8) {
        return UINT8;
    }
    else if (type == ASTNode::uint10) {
        return UINT10;
    }
    else if (type == ASTNode::uint16) {
        return UINT16;
    }
    else if (type == ASTNode::uint32) {
        return UINT32;
    }
    else if (type == ASTNode::uint64) {
        return UINT64;
    }
    else if (type == ASTNode::uint128) {
        return UINT128;
    }
    else if (type == ASTNode::uint256) {
        return UINT256;
    }
    else if (type == ASTNode::uint512) {
        return UINT512;
    }
    return INT;
}

int Transformer::getNodeTypeSize(NodeType nodeType) {
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

