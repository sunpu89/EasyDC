//
// Created by Septi on 6/16/2022.
//
#include "Interpreter.h"

bool Interpreter::isHD = false;
int Interpreter::assumptionLevel = 0;
std::vector<std::string> Interpreter::functionName;

extern std::map<std::string, std::vector<int>> allBox;
extern std::map<std::string, std::vector<int>> pboxM;
extern std::map<std::string, int> pboxMSize;
extern std::map<std::string, std::vector<int>> Ffm;

ValuePtr ASTNode::NBlock::compute(Interpreter &interpreter) {
    std::vector<ValuePtr> block;
    ValuePtr last = nullptr;
    //std::cout << "all type name : " << std::endl;
    for(auto stmt : *(this->getStmtList())) {
        // if the stmt is NFunctionCall, add it to sequence
        /*if (stmt->getTypeName() == "NExpressionStatement") {
            std::cout << stmt->jsonGen();
        } else if (stmt->getTypeName() == "NAssignment") {
            std::cout << stmt->jsonGen();
        }
        std::cout << stmt->getTypeName() << std::endl;*/

        last = stmt->compute(interpreter);
        // 这里知道实际类型是Proc，但是添加的还是父类
        if(last && last->getValueType() == ValueType::VTProcValue)
            interpreter.addProc(last);
        block.push_back(last);
    }
    return last;
}


ValuePtr ASTNode::NVariableDeclaration::compute(Interpreter &interpreter) {
    // type is an array

    if(this->type->isArray) {
        int arraySize = 1;
        std::vector<int> arrayBounds;

        bool needToDivide = false;
        for(auto bound : *(this->type->arraySize)) {
            ValuePtr ident = bound->compute(interpreter);
            if(NIdentifierPtr identifier = std::dynamic_pointer_cast<NIdentifier>(bound)) {
                // 2.2 如果此时数组有bound是n，并且此时arrayBounds不是空，那么证明此时这不是一个一维数组，需要将其拆开
                if(identifier->getName() == "n" && !arrayBounds.empty()) {
                    needToDivide = false;
                }
            }
            int valueOfBound = interpreter.value_of(ident);
            arrayBounds.push_back(valueOfBound);
            arraySize *= valueOfBound;
        }

        if(needToDivide) {
            // 如果需要拆开，那么需要新建立除了最后一个
            int newArraySize = 1;
            std::vector<int> newArrayBounds;
            newArrayBounds.push_back(arrayBounds[arrayBounds.size() - 1]);
            for(int i = 0; i < arrayBounds.size() - 1; i++)
                newArraySize *= arrayBounds[i];
            // 现在需要建立newArraySize个长度为n的参数
            std::vector<ValuePtr> finalValues;
            for(int i = 0; i < newArraySize; i++) {
                interpreter.setArraySize(this->id->getName() + "_" + std::to_string(i), newArrayBounds);

                std::vector<ValuePtr> values;
                for (int j = 0; j < newArrayBounds[0]; j++) {
                    ValuePtr valuePtr = interpreter.getValue(this->type->getName(), this->id->getName() + "_" + std::to_string(i) + "_" + std::to_string(j),
                                                             this->isParameter);
                    values.push_back(valuePtr);
                }
                ValuePtr re = std::make_shared<ArrayValue>(this->id->getName() + "_" + std::to_string(i), values);
                // 在环境中，数组中的单个元素没有拥有姓名
                interpreter.addToEnv(this->id->getName() + "_" + std::to_string(i), re);
                finalValues.push_back(re);

                if (this->isParameter) {
                    interpreter.addParameter(re);
                }

                if (this->assignmentExpr) {
                    NAssignment assignment(this->id, this->assignmentExpr);
                    return assignment.compute(interpreter);
                }
            }

            ValuePtr re = std::make_shared<ArrayValue>(this->id->getName(), finalValues);
            // 在环境中，数组中的单个元素没有拥有姓名
            interpreter.addToEnv(this->id->getName(), re);

            // for debug
//            std::cout << "NVariableDeclaration debug : " << std::endl;
//            std::cout << "this->ident->getName() : " << this->id->getName() << std::endl;
//            std::cout << "re.toString() : " << re->toString() << std::endl;


            if (this->isParameter) {
                interpreter.addParameter(re);
            }

        } else {
            interpreter.setArraySize(this->id->getName(), arrayBounds);
            std::vector<ValuePtr> values;
            // 这里修改一下，当新声明的变量为数组，且assignmentExpr不为空时，需要将assignmentExpr按照数组成员的个数依次展开嘛？
            if (this->assignmentExpr) {
                NExpressionPtr nExpressionPtr(this->assignmentExpr);

                ValuePtr temp = nExpressionPtr->compute(interpreter);
                // 赋值内容是数组，但是表示是binary operator连接的两个操作对象，如： uint1[32] sbox_out = sbox<sbox_in>;

                // Remark：实际上，还应该根据operator的种类来分别对temp对象进行处理，
                // 如，SYMBOLINDEX 和 BOXOP 就应该是不一样的处理方式
                if (temp->getValueType() == ValueType::VTInternalBinValue) {
                    // 如果temp类型是VTInternalBinValue，则要先将其转化成该类型，然后根据具体的operator进行不同的处理
                    InternalBinValue* binTemp = dynamic_cast<InternalBinValue*>(temp.get());
                    // 如果op是BOXOP，则输出value的每个元素仍然是VTInternalBinValue，并且每个元素都需要增加一个索引；
                    // 整个数组的所有元素集合也是一个VTInternalBinValue类型
                    if (binTemp->getOp() == ASTNode::Operator::BOXOP) {
                        for (int i = 0; i < arraySize; ++i) {
                            ConcreteNumValuePtr index = std::make_shared<ConcreteNumValue>("index", i);
                            InternalBinValuePtr temp2bin = std::make_shared<InternalBinValue>(
                                    this->id->getName() + "_" + std::to_string(i), temp, index,
                                    ASTNode::Operator::SYMBOLINDEX);
                            temp2bin->setVarType(Interpreter::VarTypeTrans(this->getMyType()));
                            interpreter.addToSequence(temp2bin);
                            values.push_back(temp2bin);
                        }
                    }
                    /*
                     * 如果op是XOR，首先要检查被操作对象是否是数组类型，实际上这个检查应该是类型检查系统完成。
                     * 如果被操作对象是两个，那么每个被操作对象都应该是数组
                     * 如果被操作对象是多个，每个被操作对象也都应该是数组
                     * 输出value中的每个元素，是被操作对象对应的index进行XOR的结果
                     * */
                    else if (binTemp->getOp() == ASTNode::Operator::XOR) {
                        if (binTemp->getLeft()->getValueType() == ValueType::VTArrayValue) {
                            ArrayValue *LarrayV = dynamic_cast<ArrayValue *>(binTemp->getLeft().get());
                            ArrayValue *RarrayV = dynamic_cast<ArrayValue *>(binTemp->getRight().get());
                            for (int i = 0; i < arraySize; ++i) {
                                InternalBinValuePtr temp2bin = std::make_shared<InternalBinValue>(
                                        this->id->getName() + "_" + std::to_string(i), LarrayV->getValueAt(i), RarrayV->getValueAt(i), ASTNode::Operator::XOR);
                                interpreter.addToSequence(temp2bin);
                                temp2bin->setVarType(Interpreter::VarTypeTrans(this->getMyType()));
                                values.push_back(temp2bin);
                            }
                        }
                        // 如果被操作对象有多个,就用SYMBOL_INDEX的方法将其改成数组
                        else if (binTemp->getLeft()->getValueType() == ValueType::VTInternalBinValue) {
                            std::vector<ValuePtr> res;
                            ArrayValue *RarrayV = dynamic_cast<ArrayValue *>(binTemp->getRight().get());
                            for (int i = 0; i < arraySize; ++i) {
                                ConcreteNumValuePtr index = std::make_shared<ConcreteNumValue>("index", i);
                                // ValuePtr temp = std::make_shared<ArrayValue>(rightRand->getName(), rightRand->getArrayValue());
                                InternalBinValuePtr temp2bin = std::make_shared<InternalBinValue>(
                                        "MULTI_XOR_" + std::to_string(i), binTemp->getLeft(), index,
                                        ASTNode::Operator::SYMBOLINDEX);
                                //interpreter.addToSequence(temp2bin);
                                temp2bin->setVarType(Interpreter::VarTypeTrans(this->getMyType()));
                                res.push_back(temp2bin);
                            }
                            for (int i = 0; i < arraySize; ++i) {
                                InternalBinValuePtr temp2bin = std::make_shared<InternalBinValue>(
                                        this->id->getName() + "_" + std::to_string(i), res[i], RarrayV->getValueAt(i), ASTNode::Operator::XOR);
                                temp2bin->setVarType(Interpreter::VarTypeTrans(this->getMyType()));
                                interpreter.addToSequence(temp2bin);
                                values.push_back(temp2bin);
                            }
                        }
                        // 如果不满足上述所有情况，报错
                        else {
                            assert(false);
                        }
                    }
                    // 如果是 pboxm * uints[n], 则返回uints[n]类型
                    else if (binTemp->getOp() == ASTNode::Operator::FFTIMES) {
                        if (binTemp->getLeft()->getValueType() == ValueType::VTBoxValue) {
                            for (int i = 0; i < arraySize; ++i) {
                                ConcreteNumValuePtr index = std::make_shared<ConcreteNumValue>("index", i);
                                InternalBinValuePtr temp2bin = std::make_shared<InternalBinValue>(
                                        this->id->getName() + "_" + std::to_string(i), temp, index,
                                        ASTNode::Operator::SYMBOLINDEX);
                                interpreter.addToSequence(temp2bin);
                                temp2bin->setVarType(Interpreter::VarTypeTrans(this->getMyType()));
                                values.push_back(temp2bin);
                            }
                        }
                        else {
                            assert(false);
                        }
                    }
                    // 实际上，循环左移循环右移在binaryoperator::compute()里直接被evaluate了
                    // 循环右移
                    /*else if (binTemp->getOp() == ASTNode::Operator::RRSH) {
                        std::string tt = binTemp->getRight()->getName();
                        std::string tt1 = binTemp->getRight()->getName();
                        NBinaryOperator* binaryOperator = dynamic_cast<NBinaryOperator*>(nExpressionPtr.get());

                        std::string tt3 = binTemp->getRight()->getName();
                    }*/
                    // 后续可以添加其他类型操作符的处理
                    else {
                        assert(false);
                    }
                }
                // 赋值内容是数组，如： uint1[32] a = View(input, 0, 31);
                else if (temp->getValueType() == ValueType::VTArrayValue) {
                    values = dynamic_cast<ArrayValue *>(temp.get())->getArrayValue();
                }
                // 后续可以添加其他类型的处理
                else
                    assert(false);
            }
            // 否则，当assignmentExpr为空时，我们就不需要那根据assignmentExpr的表达式来依次对数组的所有元素赋值。
            else {
                for (int i = 0; i < arraySize; i++) {
                    ValuePtr valuePtr = interpreter.getValue(this->type->getName(),
                                                             this->id->getName() + "_" + std::to_string(i),
                                                             this->isParameter);
                    valuePtr->setVarType(Interpreter::VarTypeTrans(this->getMyType()));
                    values.push_back(valuePtr);
                }
            }
            ValuePtr re = std::make_shared<ArrayValue>(this->id->getName(), values);
            re->setVarType(Interpreter::VarTypeTrans(this->getMyType()));
            // std::cout << "name : " << this->id->getName() << "\ntype : " << this->type->getName() << std::endl;
            // 在环境中，数组中的单个元素没有拥有姓名
            interpreter.addToEnv(this->id->getName(), re);

            // for debug
            //std::cout << "NVariableDeclaration debug : " << std::endl;
            //std::cout << "this->ident->getName() : " << this->id->getName() << std::endl;

            if (this->isParameter) {
                interpreter.addParameter(re);
            }

            /*if (this->assignmentExpr) {
                NAssignment assignment(this->id, this->assignmentExpr);
                return assignment.compute(interpreter);
            }*/
        }

        return nullptr;
    }
    else {
        std::string name = this->id->getName();
        ValuePtr value = interpreter.getValue(this->type->getName(), name, this->isParameter);
        value->setVarType(interpreter.VarTypeTrans(this->getMyType()));
        interpreter.addToEnv(name, value);

        if(this->isParameter) {
            interpreter.addParameter(value);
        }

        if(this->assignmentExpr) {
            /*if (name == "sbox_in")
                std::cout << "stop here !" << std::endl;*/
            // assert(interpreter.getFromEnv(name) == nullptr);
            NAssignment assignment(this->id, this->assignmentExpr);
            // assignment 需要加到env和sequence吗？ 7.06.2022
            ValuePtr valuePtr = assignment.compute(interpreter);
            // 在赋值时就已经加入到了env中，因为assignment::compute的返回值为空指针，所以会将原来添加到的变量覆盖为空指针
            interpreter.addToEnv(this->id->getName(), valuePtr);
            return valuePtr;
        }
        return nullptr;
    }
}


ValuePtr ASTNode::NFunctionDeclaration::compute(Interpreter &interpreter) {
//    cout << "Compute: Generating function declaration of " << this->id->getName() << endl;
    std::string functionName = this->id->getName();

    // 对于每一个新声明的函数，都需要向其添加所有的全局变量作为该函数env的一部分。
    std::map<std::string, ValuePtr> temptopTnv = interpreter.getEnv();
    interpreter.pushBlock();
    for (auto env : temptopTnv) {
        interpreter.addToEnv(env.first, env.second);
    }

    // get parameters
    for(auto pa : *(this->var_list)) {
        pa->compute(interpreter);
    }

    std::vector<ValuePtr> parameters = interpreter.getParameter();
    //interpreter.addParameter(parameters);
    // for debug
    /*std::cout << "main parameters : " << std::endl;
    for (auto param : parameters) {
        std::cout << param->getName() << std::endl;
    }*/

    /*if(this->id->getName() == "main") {
        std::cout << "main parameters : " << std::endl;
        for (const auto& param : parameters) {
            std::cout << param->getName() << std::endl;
        }
    }*/

    // annotation for debug by sunpu
    /*if(this->id->getName() == "main") {
        assert(parameters.size() == (this->var_list)->size());
        for(int i = 0; i < (this->var_list)->size(); i++) {
            auto pa = (this->var_list)->at(i);
            if(pa->getMyType() == "private") {
                if(ArrayValuePtr arrayValuePtr = dynamic_pointer_cast<ArrayValue>(parameters[i])) {
                    for(auto ele : arrayValuePtr->getArrayValue())
                        interpreter.addNameOfKey(ele->getName());
                } else if(ParameterValuePtr parameterValue = dynamic_pointer_cast<ParameterValue>(parameters[i])){
                    interpreter.addNameOfKey(parameterValue->getName());
                } else {
                    assert(false);
                }
            } else if(pa->getMyType() == "public") {
                if(ArrayValuePtr arrayValuePtr = dynamic_pointer_cast<ArrayValue>(parameters[i])) {
                    for(auto ele : arrayValuePtr->getArrayValue())
                        interpreter.addNameOfPlain(ele->getName());
                } else if(ParameterValuePtr parameterValue = dynamic_pointer_cast<ParameterValue>(parameters[i])){
                    interpreter.addNameOfPlain(parameterValue->getName());
                } else {
                    assert(false);
                }
            }
        }
    }*/

    // get body
    ValuePtr body = this->block->compute(interpreter);
    interpreter.addToSequence(body);

    ValuePtr returns = interpreter.getCurrentReturnValue();

    std::vector<ValuePtr> assumptions = interpreter.getAssumptions();

    // assumption该parse还是parse，至于存不存放在函数中，就在这里判断吧

    /*if(Interpreter::assumptionLevel == 1 || Interpreter::assumptionLevel == 0) {
        // 没有假设
        assumptions.clear();
    } else if(Interpreter::assumptionLevel == 3) {
        // 走全部假设，不用管
    } else if(Interpreter::assumptionLevel == 2) {
        // 走部分假设
        bool flag = false;
        for(auto ele : Interpreter::functionName) {
            if(ele == functionName) {
                flag = true;
                break;
            }
        }
        if(!flag) {
            assumptions.clear();
        }
    }*/

    if(functionName == "preshare") {
        std::vector<ValuePtr>& sequence = interpreter.getSequence();
        for(auto it = sequence.begin(); it != sequence.end() - 1; ) {
            if(it->get()->getValueType() == ValueType::VTInternalBinValue) {
                it = sequence.erase( it );
            } else{
                ++it;
            }
        }
//        InternalBinValue* binValue = dynamic_cast<InternalBinValue*>(sequence.end()->get());
//        assert(binValue);
    }

    ProcedurePtr proc = std::make_shared<Procedure>(functionName, parameters,assumptions, interpreter.getEnv(),
                                               interpreter.getSequence(), returns, interpreter.getHDSequence());

    interpreter.addToIntenal(interpreter.getSequence().size());

    // debug
    /*std::cout << "all function's sequences : " << std::endl;
    for(auto ele : interpreter.getSequence()) {
        cout << ele->toString() << endl;
    }*/

    //std::cout << "function tostring : \n" << proc->toString() << std::endl;

    return std::make_shared<ProcValue>(functionName, proc);
}


ValuePtr ASTNode::NRoundFunctionDeclaration::compute(Interpreter &interpreter) {
    /*std::cout << "Compute: Generating function declaration : "
        << "\ntype : " << this->type->getName() << " " << this->type->getTypeName() << " " << this->type->isArray
        << "\nid : " << this->id->getName() << std::endl;*/

    std::string functionName = this->id->getName();

    // 对于每一个新声明的函数，都需要向其添加所有的全局变量作为该函数env的一部分。
    // 从而可以在函数体中进行某些处理时，可以直接调用
    std::map<std::string, ValuePtr> temptopTnv = interpreter.getblockStack()[0]->env;
    interpreter.pushBlock();
    for (auto env : temptopTnv) {
        interpreter.addToEnv(env.first, env.second);
    }

    // process arguments
    this->round->compute(interpreter);
    this->sk->compute(interpreter);
    this->p->compute(interpreter);


    std::vector<ValuePtr> parameters = interpreter.getParameter();

    // get body
    ValuePtr body = this->block->compute(interpreter);
    interpreter.addToSequence(body);

    ValuePtr returns = interpreter.getCurrentReturnValue();

    // 这里的assumption会在后面procedureptr instance声明初始化时使用，
    // 因此虽然对我来说没啥作用，也暂时留着了。
    std::vector<ValuePtr> assumptions = interpreter.getAssumptions();

    // assumption该parse还是parse，至于存不存放在函数中，就在这里判断吧

    /*if(Interpreter::assumptionLevel == 1 || Interpreter::assumptionLevel == 0) {
        // 没有假设
        assumptions.clear();
    } else if(Interpreter::assumptionLevel == 3) {
        // 走全部假设，不用管
    } else if(Interpreter::assumptionLevel == 2) {
        // 走部分假设
        bool flag = false;
        for(auto ele : Interpreter::functionName) {
            if(ele == functionName) {
                flag = true;
                break;
            }
        }
        if(!flag) {
            assumptions.clear();
        }
    }*/

    ProcedurePtr proc = std::make_shared<Procedure>(functionName, parameters,assumptions, interpreter.getEnv(),
                                               interpreter.getSequence(), returns, interpreter.getHDSequence());
    proc->setIsRndf();
    interpreter.addToIntenal(interpreter.getSequence().size());

    // debug
    /*std::cout << "all function's sequences : " << std::endl;
    for(auto ele : interpreter.getSequence()) {
        cout << ele->toString() << endl;
    }*/

    //std::cout << "function tostring : \n" << proc->toString() << std::endl;

    ValuePtr procRtn = std::make_shared<ProcValue>(functionName, proc);
    procRtn->setVarType(Interpreter::VarTypeTrans(this->type->getName()));
    return procRtn;
}


ValuePtr ASTNode::NSboxFunctionDeclaration::compute(Interpreter &interpreter) {
    std::string functionName = this->id->getName();

    // 对于每一个新声明的函数，都需要向其添加所有的全局变量作为该函数env的一部分。
    // 从而可以在函数体中进行某些处理时，可以直接调用
    //std::map<std::string, ValuePtr> temptopTnv = interpreter.getEnv();
    std::map<std::string, ValuePtr> temptopTnv = interpreter.getblockStack()[0]->env;
    interpreter.pushBlock();
    for (auto env : temptopTnv) {
        interpreter.addToEnv(env.first, env.second);
    }

    // process arguments
    this->input->compute(interpreter);

    std::vector<ValuePtr> parameters = interpreter.getParameter();

    // get body
    ValuePtr body = this->block->compute(interpreter);
    interpreter.addToSequence(body);

    ValuePtr returns = interpreter.getCurrentReturnValue();

    // 这里的assumption会在后面procedureptr instance声明初始化时使用，
    // 因此虽然对我来说没啥作用，也暂时留着了。
    std::vector<ValuePtr> assumptions = interpreter.getAssumptions();

    ProcedurePtr proc = std::make_shared<Procedure>(functionName, parameters,assumptions, interpreter.getEnv(),
                                                    interpreter.getSequence(), returns, interpreter.getHDSequence());
    proc->setIsSboxf();
    interpreter.addToIntenal(interpreter.getSequence().size());

    ValuePtr procRtn = std::make_shared<ProcValue>(functionName, proc);
    procRtn->setVarType(Interpreter::VarTypeTrans(this->type->getName()));
    return procRtn;
}


ValuePtr ASTNode::NCipherFunctionDeclaration::compute(Interpreter &interpreter) {
    std::string functionName = this->id->getName();

    // 对于每一个新声明的函数，都需要向其添加所有的全局变量作为该函数env的一部分。
    // 从而可以在函数体中进行某些处理时，可以直接调用
    //std::map<std::string, ValuePtr> temptopTnv = interpreter.getEnv();
    std::map<std::string, ValuePtr> temptopTnv = interpreter.getblockStack()[0]->env;
    interpreter.pushBlock();
    for (auto env : temptopTnv) {
        interpreter.addToEnv(env.first, env.second);
    }

    // process arguments
    this->k->compute(interpreter);
    this->p->compute(interpreter);

    std::vector<ValuePtr> parameters = interpreter.getParameter();

    // get body
    ValuePtr body = this->block->compute(interpreter);
    interpreter.addToSequence(body);

    ValuePtr returns = interpreter.getCurrentReturnValue();

    // 这里的assumption会在后面procedureptr instance声明初始化时使用，
    // 因此虽然对我来说没啥作用，也暂时留着了。
    std::vector<ValuePtr> assumptions = interpreter.getAssumptions();

    ProcedurePtr proc = std::make_shared<Procedure>(functionName, parameters,assumptions, interpreter.getEnv(),
                                                    interpreter.getSequence(), returns, interpreter.getHDSequence());
    proc->setIsFn();
    interpreter.addToIntenal(interpreter.getSequence().size());

    ValuePtr procRtn = std::make_shared<ProcValue>(functionName, proc);
    procRtn->setVarType(Interpreter::VarTypeTrans(this->type->getName()));
    return procRtn;
}


ValuePtr ASTNode::NReturnStatement::compute(Interpreter &interpreter) {
//    cout << "Generating return statement" << endl;
    ValuePtr returns = this->expr->compute(interpreter);
    interpreter.setCurrentReturnValue(returns);
    return nullptr;
}


ValuePtr ASTNode::NIdentifier::compute(Interpreter &interpreter) {
    std::string nodeName = this->getName();

    ValuePtr valuePtr = interpreter.getFromEnv(nodeName);

    assert(valuePtr);

    return valuePtr;

//    if(valuePtr == nullptr) {
//        vector<string> arrayNames = interpreter.getArrayNames(nodeName);
//        vector<ValuePtr> arrayValues;
//        for(auto ele : arrayNames) {
//            arrayValues.push_back(interpreter.getFromEnv(ele));
//        }
//        return make_shared<ArrayValue>(arrayValues);
//    } else {
//        return valuePtr;
//    }

//    return interpreter.getFromEnv(nodeName);
}

ValuePtr ASTNode::NExpressionStatement::compute(Interpreter &interpreter) {
    return this->expr->compute(interpreter);
}


ValuePtr ASTNode::NAssignment::compute(Interpreter &interpreter) {
//    cout << "Generating assignment of " << this->LHS->getName() << " = " << endl;

    /*if (this->LHS->getName() == "s_out0")
        std::cout << "stop here!" << std::endl;*/

    ValuePtr expValue = this->RHS->compute(interpreter);

    // type system
    if (interpreter.getFromEnv(this->LHS->getName())->getVarType() != expValue->getVarType()) {
        // 若两个都为数组，则没有类型问题
        ArrayValue* arrayValueL = dynamic_cast<ArrayValue*>(interpreter.getFromEnv(this->LHS->getName()).get());
        ArrayValue* arrayValueR = dynamic_cast<ArrayValue*>(expValue.get());
        ConcreteNumValue* expConcrete = dynamic_cast<ConcreteNumValue*>(expValue.get());
        // 赋值语句时，右边可以为具体数值
        if (arrayValueL != nullptr and arrayValueR != nullptr) {

        }
        else if (expConcrete == nullptr) {
            std::cout << "LHS type : " << interpreter.getFromEnv(this->LHS->getName())->getVarType() << std::endl;
            std::cout << "exp type : " << expValue->getVarType() << std::endl;
            std::cout << "TYPE ERROR : The assignment object is not of the same type as the assigned object"
                      << std::endl;
            abort();
        }
    }

    if(interpreter.isHD) {
        // 如果是在HD model下，就要判断是不是重复赋值了
        ValuePtr oldValue = interpreter.getFromEnv(this->LHS->getName());
        if (oldValue) {
            ArrayValue* arrayValueOld = dynamic_cast<ArrayValue*>(oldValue.get());
            if(arrayValueOld) {
                ArrayValue* arrayValueNew = dynamic_cast<ArrayValue*>(expValue.get());
                for(int i = 0; i < arrayValueOld->getArrayValue().size(); i++) {
                    if(arrayValueOld->getValueAt(i)) {
                        ValuePtr hdValue = std::make_shared<InternalBinValue>("hd", arrayValueOld->getValueAt(i), arrayValueNew->getValueAt(i), ASTNode::Operator::XOR);
                        arrayValueOld->getValueAt(i)->addParents(hdValue);
                        arrayValueNew->getValueAt(i)->addParents(hdValue);
                        interpreter.addToHDSequence(hdValue);
                    }
                }
            } else {
                // 如果oldValue不是具体值的话
                if(oldValue->getValueType() != ValueType::VTConcreteNumValue) {
                    // 重复赋值
                    ValuePtr hdValue = std::make_shared<InternalBinValue>("hd", expValue, oldValue, ASTNode::Operator::XOR);
                    expValue->addParents(hdValue);
                    oldValue->addParents(hdValue);
                    interpreter.addToHDSequence(hdValue);
                }
            }
        }
    }

    // 当被赋值对象不是数组时，直接添加到env
    // 可以在当前的env里查找被赋值的对象，来看该对象的类型是否是数组

    // 如果被赋值的LHS已经在env, 且该对象的类型不是数组，则可以直接添加到env

    // debug 7.05 先去掉，不管什么情况都加到env中先
    // if (interpreter.getFromEnv(this->LHS->getName()) != nullptr and interpreter.getFromEnv(this->LHS->getName())->getValueType() != ValueType::VTArrayValue)
    interpreter.addToEnv(this->LHS->getName(), expValue);

    if(expValue->getValueType() == ValueType::VTConcreteNumValue) {
        // ConcreteNumValue不需要加入到sequence中去
    }else if(expValue->getValueType() == ValueType::VTInternalBinValue) {
        // 表达式的操作对象不是array时，在RHS->compute时就已经被添加紧sequence
        // 当操作对象是array时，要在这里按照array的size展开，并依次放入sequence
        InternalBinValue* binExpValue = dynamic_cast<InternalBinValue*>(expValue.get());
        // 首先判断被操作对象是否是数组
        if (binExpValue->getRight()->getValueType() == ValueType::VTArrayValue) {
            // 如果被操作对象是数组，那么也新建一个数组对象用以存放被操作的结果，后续添加到env中
            std::vector<ValuePtr> arrayEles;
            if (binExpValue->getOp() == ASTNode::Operator::BOXOP) {
                // 当operator为BOXOP时，输出array的size和输入Right的size相同
                ArrayValue *binExpValueRight = dynamic_cast<ArrayValue *>(binExpValue->getRight().get());
                for (int i = 0; i < binExpValueRight->getArrayValue().size(); ++i) {
                    ConcreteNumValuePtr index = std::make_shared<ConcreteNumValue>("index", i);
                    InternalBinValuePtr outbin = std::make_shared<InternalBinValue>(
                            this->LHS->getName() + "_" + std::to_string(i), expValue, index, ASTNode::Operator::SYMBOLINDEX);
                    interpreter.addToSequence(outbin);
                    arrayEles.push_back(outbin);
                }
            }
                // 如果op是XOR，首先要检查被操作对象是否是数组类型，实际上这个检查应该是类型检查系统完成。
                // 输出value中的每个元素，是被操作对象对应的index进行XOR的结果
            else if (binExpValue->getOp() == ASTNode::Operator::XOR) {
                ArrayValue *LarrayV = dynamic_cast<ArrayValue *>(binExpValue->getLeft().get());
                ArrayValue *RarrayV = dynamic_cast<ArrayValue *>(binExpValue->getRight().get());
                for (int i = 0; i < LarrayV->getArrayValue().size(); ++i) {
                    InternalBinValuePtr outbin = std::make_shared<InternalBinValue>(
                            this->LHS->getName() + "_" + std::to_string(i), LarrayV->getValueAt(i), RarrayV->getValueAt(i),
                            ASTNode::Operator::XOR);
                    interpreter.addToSequence(outbin);
                    arrayEles.push_back(outbin);
                }
            }
            // 2022.8.9, 新增运算ADD，用于适配ARX结构的box
            // 如果op是ADD，首先要检查被操作对象是否是数组类型，实际上这个检查应该是类型检查系统完成。
            // 输出value中的每个元素，是被操作对象对应的index进行ADD的结果
            else if (binExpValue->getOp() == ASTNode::Operator::ADD) {
                ArrayValue *LarrayV = dynamic_cast<ArrayValue *>(binExpValue->getLeft().get());
                ArrayValue *RarrayV = dynamic_cast<ArrayValue *>(binExpValue->getRight().get());
                for (int i = 0; i < LarrayV->getArrayValue().size(); ++i) {
                    InternalBinValuePtr outbin = std::make_shared<InternalBinValue>(
                            this->LHS->getName() + "_" + std::to_string(i), LarrayV->getValueAt(i), RarrayV->getValueAt(i),
                            ASTNode::Operator::ADD);
                    interpreter.addToSequence(outbin);
                    arrayEles.push_back(outbin);
                }
            }
            else {
                assert(false);
            }
            ValuePtr valuePtr = std::make_shared<ArrayValue>(this->LHS->getName(), arrayEles);
            valuePtr->setVarType(interpreter.getFromEnv(this->LHS->getName())->getVarType());
            if (binExpValue->getOp() == ASTNode::Operator::BOXOP)
                valuePtr->setIsViewOrTouint();
            interpreter.addToEnv(this->LHS->getName(), valuePtr);
        }
        // 当被操作对象不是数组，那么输出对象也不应该是数组
        else {
            // 当操作符为BOXOP时，输出的依旧是一个InternalBinValue
            if (binExpValue->getOp() == ASTNode::Operator::BOXOP) {
                expValue->setName(this->LHS->getName());
                interpreter.addToSequence(expValue); // 在NBoxOperation::compute中已经被添加至sequence，不需要重复添加
                interpreter.addToEnv(this->LHS->getName(), expValue);
            }
            else if (binExpValue->getOp() == ASTNode::Operator::XOR) {
                expValue->setName(this->LHS->getName());
                interpreter.addToSequence(expValue); // 在NBoxOperation::compute中已经被添加至sequence，不需要重复添加
                interpreter.addToEnv(this->LHS->getName(), expValue);
            }
            else {
                assert(false);
            }
            return expValue;
        }
    }
    else if(expValue->getValueType() == ValueType::VTInternalUnValue) {
        // 当expValue是VTInternalUnValue
        InternalUnValue* unExpValue = dynamic_cast<InternalUnValue*>(expValue.get());
        // 首先判断被操作对象是否是数组
        if (unExpValue->getRand()->getValueType() == ValueType::VTArrayValue) {
            // 根据被操作对象和操作符，输出对象来决定输出对象的类型，然后加到env中
            if (unExpValue->getOp() == ASTNode::Operator::TOUINT) {
                expValue->setIsViewOrTouint();
                expValue->setName(this->LHS->getName());
                interpreter.addToSequence(expValue);
                interpreter.addToEnv(this->LHS->getName(), expValue);
            }
            return expValue;
        }
            // 不是就直接退出
        else {
            assert(false);
        }


    }
    else {
        interpreter.addToSequence(expValue);
    }

    return expValue;

}


// 一些可以evaluate的操作符在经过本函数以后，直接返回evaluate后的结果
ValuePtr ASTNode::NBinaryOperator::compute(Interpreter &interpreter) {

    // for bebug
    /*if (this->getOp() == ASTNode::Operator::FFTIMES and this->rhs->getTypeName() == "NIdentifier")
        std::cout << "stop here !" << std::endl;*/

    ValuePtr L = this->lhs->compute(interpreter);
    assert(L);

    /*if (L->getName() == "p_out23")
        std::cout << "stop here !" << std::endl;*/

    ValuePtr R = this->rhs->compute(interpreter);
    assert(R);

    ValuePtr res = std::make_shared<InternalBinValue>("", L, R, this->op);
    if (L->getVarType() != ASTNode::null)
        res->setVarType(L->getVarType());
    else if (R->getVarType() != ASTNode::null)
        res->setVarType(R->getVarType());

//    ConcreteNumValue* Lvalue = dynamic_cast<ConcreteNumValue*>(L.get());
    bool isLeftConcreteNum = L->getValueType() == VTConcreteNumValue ? true : false;
//    ConcreteNumValue* Rvalue = dynamic_cast<ConcreteNumValue*>(R.get());
    bool isRightConcreteNum = R->getValueType() == VTConcreteNumValue ? true : false;
    if(isLeftConcreteNum && isRightConcreteNum) {
        int lvalue = interpreter.value_of(L);
        int rvalue = interpreter.value_of(R);
        if(this->getOp() == ASTNode::Operator::ADD) {
            return std::make_shared<ConcreteNumValue>("", lvalue + rvalue);
        } else if(this->getOp() == ASTNode::Operator::MINUS){
            return std::make_shared<ConcreteNumValue>("", lvalue - rvalue);
        } else if(this->getOp() == ASTNode::Operator::FFTIMES){
            return std::make_shared<ConcreteNumValue>("", lvalue * rvalue);
        } else if(this->getOp() == ASTNode::Operator::DIVIDE){
            return std::make_shared<ConcreteNumValue>("", lvalue / rvalue);
        } else if(this->getOp() == ASTNode::Operator::LSH){
            return std::make_shared<ConcreteNumValue>("", lvalue << rvalue);
        } else if(this->getOp() == ASTNode::Operator::AND){
            return std::make_shared<ConcreteNumValue>("", lvalue & rvalue);
        } else if(this->getOp() == ASTNode::Operator::MOD){
            return std::make_shared<ConcreteNumValue>("", lvalue % rvalue);
        } else {
            assert(false);
        }
    }

    if (this->getOp() == ASTNode::Operator::RRSH) {
        ArrayValue* arrayValue = dynamic_cast<ArrayValue*>(L.get());
        assert(arrayValue);
        int displacement = interpreter.value_of(R);
        std::vector<ValuePtr> array = arrayValue->getArrayValue();
        std::vector<ValuePtr> rtn;
        for (int i = 0; i < displacement; ++i) {
            rtn.push_back(array[array.size() - displacement + i]);
        }
        for (int i = 0; i < array.size() - displacement; ++i) {
            rtn.push_back(array[i]);
        }
        ValuePtr arrayvalue = std::make_shared<ArrayValue>(L->getName(), rtn);
        arrayvalue->setVarType(arrayValue->getVarType());
        return arrayvalue;
    }
    else if (this->getOp() == ASTNode::Operator::RLSH) {
        ArrayValue* arrayValue = dynamic_cast<ArrayValue*>(L.get());
        assert(arrayValue);
        int displacement = interpreter.value_of(R);
        std::vector<ValuePtr> array = arrayValue->getArrayValue();
        std::vector<ValuePtr> rtn;
        for (int i = 0; i < array.size() - displacement; ++i) {
            rtn.push_back(array[displacement + i]);
        }
        for (int i = 0; i < displacement; ++i) {
            rtn.push_back(array[i]);
        }
        ValuePtr arrayvalue = std::make_shared<ArrayValue>(L->getName(), rtn);
        arrayvalue->setVarType(arrayValue->getVarType());
        return arrayvalue;
    }

    // pboxm
    BoxValue* boxValue = dynamic_cast<BoxValue*>(L.get());
    if (boxValue != nullptr) {
        if (boxValue->getBoxType() == "pboxm") {
            // 这里需要进行矩阵乘向量的计算，需要先获取env中的ffm，从而知道对应的有限域乘法计算规则，
            // 然后按照计算规则，依次将矩阵的元素和对应的vector的每个元素相乘
            std::string mName = boxValue->getName();
            ValuePtr mFfm = interpreter.getEnv()[mName + "_ffm"];
            BoxValue* boxFfm = dynamic_cast<BoxValue*>(mFfm.get());
            ArrayValue* rVector = dynamic_cast<ArrayValue*>(R.get());
            // 根据ffm中的一个元素的类型，或者对应优先于计算规则来判断是哪种计算
            if (boxValue->getValueAt(0)->getVarType() == ASTNode::uint1) {
                // 现在觉得好像没有必要在这里进行规则判断，而是应该将对应的pboxm和对应的ffm一起保存起来，这样在进行模型转换时直接统一进行计算即可。
            }

            // 直接存储对应的pboxm和ffm
            std::vector<int> pboxmValue;
            std::vector<ValuePtr> pboxmV = boxValue->getBoxValue();
            for (auto ele : pboxmV) {
                ConcreteNumValue* eleConcrete = dynamic_cast<ConcreteNumValue*>(ele.get());
                pboxmValue.push_back(eleConcrete->getNumer());
            }
            pboxM["pboxm" + mName] = pboxmValue;
            pboxMSize["pboxm" + mName] = interpreter.getVarTypeSize(boxValue->getVarType());

            std::vector<int> ffmValue;
            std::vector<ValuePtr> ffmV = boxFfm->getBoxValue();
            for (auto ele : ffmV) {
                ConcreteNumValue* eleConcrete = dynamic_cast<ConcreteNumValue*>(ele.get());
                ffmValue.push_back(eleConcrete->getNumer());
            }
            // 这里ffm保存的名字需要和pboxm中的一致，方便后续查找
            Ffm["pboxm" + mName] = ffmValue;
        }
        else {
            assert(false);
        }
    }


    L->addParents(res);
    R->addParents(res);

    // TYPE SYSTEM
    // 这里的type system实际上有个问题，就是当binaryoperator连接了多个操作对象时，如何对每个操作对象的类型都进行判断呢？
    if (L->getVarType() != R->getVarType()) {
        ConcreteNumValue* concreteNumValue = dynamic_cast<ConcreteNumValue*>(R.get());
        if (concreteNumValue == nullptr) {
            std::cout << "TYPE ERROR : Inconsistent object types for binary operations" << std::endl;
            abort();
        }
    }

    // 如果 L 和 R 都不是数组类型的，那么就直接将这两个的操作加入到sequence
    // 否则，应该将数组对应每个元素的操作都加入到sequence
    if (L->getValueType() != ValueType::VTArrayValue and R->getValueType() != ValueType::VTArrayValue)
        interpreter.addToSequence(res);

    return res;
}


ValuePtr ASTNode::NUnaryOperator::compute(Interpreter &interpreter) {
    ValuePtr rand = this->lhs->compute(interpreter);
    ValuePtr res = std::make_shared<InternalUnValue>("", rand, this->op);
    rand->addParents(res);
    interpreter.addToSequence(res);
    return res;
}


// 根据array的名称和index，从env中返回arrayindex的值
ValuePtr ASTNode::NArrayIndex::compute(Interpreter &interpreter) {
    std::string name = this->arrayName->getName();

    // 这里我们改一下ArrayIndex的compute逻辑：
    // 当dimons有一个元素的时候，即一维访问，直接进行compute即可;
    // 当dimons有两个元素的时候，先访问第一个元素所表示的结果，再访问第二个元素的结果.
    // 值得注意的是，尽管我们的语法支持二维数组，但是二维数组只能是pboxm或者ffm，而这两个是不会直接进行数组访问的。
    // 能直接进行数组访问的只有一维数组，所以我们分为一维和二维的两种元素访问方式
    if (this->dimons->size() == 2) {
        NExpressionPtr d1 = dimons->at(0);
        NExpressionPtr d2 = dimons->at(1);
        NExpressionListPtr tempDimons = std::make_shared<NExpressionList>(0);
        tempDimons->push_back(d1);

        NArrayIndex temp(this->arrayName, 1);
        temp.dimons = tempDimons;
        ValuePtr tempCompute = temp.compute(interpreter);
        interpreter.addToEnv(tempCompute->getName(), tempCompute);

        NIdentifier tempIdent(tempCompute->getName());
        this->setArrayName(std::make_shared<NIdentifier>(tempIdent));
        NExpressionListPtr tempDimons2 = std::make_shared<NExpressionList>(0);
        tempDimons2->push_back(d2);
        this->dimons = tempDimons2;
        name = tempCompute->getName();
    }

    auto sizeVec = interpreter.getArraySize(name);

    ValuePtr array = interpreter.getFromEnv(name);
    ArrayValue *arrayV = dynamic_cast<ArrayValue *>(array.get());
    // assert(arrayV);
    // 当arrayV == nullptr 时，说明是对uints的整数取某位bit。那么就要将其转化为对应该uints中s个数的size的ArrayValue类型
    if (arrayV == nullptr) {
        // 这里应该分为两种情况，一是boxop的InternalBinValue，如 sbox<uint4>,此时将right看作数组,right是touint，是一个InternalUnValue
        // added in 8.3 有第三种情况，即整体array都是touint （突然发现touint的op也是InternalUnValue）
        InternalBinValue* array2Bin = dynamic_cast<InternalBinValue*>(array.get());
        int size;
        std::vector<ValuePtr> res;

        if (array2Bin != nullptr) {
            if (array2Bin->getRight()->getValueType() == ValueType::VTInternalUnValue) {
                InternalUnValue *right = dynamic_cast<InternalUnValue *>(array2Bin->getRight().get());
                ArrayValue *rightRand = dynamic_cast<ArrayValue *>(right->getRand().get());
                size = rightRand->getArrayValue().size();
                for (int i = 0; i < size; ++i) {
                    ConcreteNumValuePtr index = std::make_shared<ConcreteNumValue>("index", i);
                    // ValuePtr temp = std::make_shared<ArrayValue>(rightRand->getName(), rightRand->getArrayValue());
                    InternalBinValuePtr temp2bin = std::make_shared<InternalBinValue>(
                            rightRand->getName() + "_" + std::to_string(i), array, index,
                            ASTNode::Operator::SYMBOLINDEX);
                    //interpreter.addToSequence(temp2bin);
                    res.push_back(temp2bin);
                }
            }
                // 另一种是情况是已经加过 SYMBOLINDEX op的 InternalBinValue，此时应该在此基础上再增加一层
                // 此时的SYMBOLINDEX op已经经过ArrayIndex::compute处理过一次，因此，可以根据name移除最后作为标识的index，寻找env中的同名var，从而根据它的var type来决定数组的大小
            else if (array2Bin->getRight()->getValueType() == ValueType::VTInternalUnValue) {
                std::string parentsName = name.substr(0, name.size()-1);
                ValuePtr parents = interpreter.getFromEnv(parentsName);
                size = interpreter.getVarTypeSize(parents->getVarType());
                for (int i = 0; i < size; ++i) {
                    ConcreteNumValuePtr index = std::make_shared<ConcreteNumValue>("index", i);
                    // ValuePtr temp = std::make_shared<ArrayValue>(rightRand->getName(), rightRand->getArrayValue());
                    InternalBinValuePtr temp2bin = std::make_shared<InternalBinValue>(
                            name + "_" + std::to_string(i), array, index,
                            ASTNode::Operator::SYMBOLINDEX);
                    //interpreter.addToSequence(temp2bin);
                    res.push_back(temp2bin);
                }
            }
            // @2022.10.24
            // 这里我们是将array2Bin转为一个数组，其中每个元素都应该是uint1，
            // 如果array2Bin非空，那么我们似乎只需要根据array2Bin的type类型即可决定数组的size
            else {
                size = interpreter.getVarTypeSize(array2Bin->getVarType());
                for (int i = 0; i < size; ++i) {
                    ConcreteNumValuePtr index = std::make_shared<ConcreteNumValue>("index", i);
                    // ValuePtr temp = std::make_shared<ArrayValue>(rightRand->getName(), rightRand->getArrayValue());
                    InternalBinValuePtr temp2bin = std::make_shared<InternalBinValue>(
                            name + "_" + std::to_string(i), array, index,
                            ASTNode::Operator::SYMBOLINDEX);
                    //interpreter.addToSequence(temp2bin);
                    res.push_back(temp2bin);
                }
            }
        }
        // 有些情况下array无法被转化成InternalBinValue
        else if (array2Bin == nullptr) {
            // 当array为concreteNumValue时, 需要构建一个数组类型，其中每个元素都是一个internalBinValue，用SYMBOLINDEX连接，而被取值的对象是一个concreteNumValue
            if (array->getValueType() == ValueType::VTConcreteNumValue) {
                // 首先从env中根据array的名字取出其对应的value对象，以提取它的valType，从而确定构建数组的size
                ValuePtr con = interpreter.getFromEnv(this->arrayName->getName());
                size = interpreter.getVarTypeSize(con->getVarType());
                for (int i = 0; i < size; ++i) {
                    ConcreteNumValuePtr index = std::make_shared<ConcreteNumValue>("index", i);
                    // ValuePtr temp = std::make_shared<ArrayValue>(rightRand->getName(), rightRand->getArrayValue());
                    InternalBinValuePtr temp2bin = std::make_shared<InternalBinValue>(
                            this->arrayName->getName() + "_" + std::to_string(i), array, index,
                            ASTNode::Operator::SYMBOLINDEX);
                    //interpreter.addToSequence(temp2bin);
                    // 当array为concreteNumValue时, 被访问的每个元素的类型被设置为uint1
                    temp2bin->setVarType(ASTNode::uint1);
                    res.push_back(temp2bin);
                }
            }
            // 当array为arrayValueIndex时，
            else if (array->getValueType() == ValueType::VTArrayValueIndex) {
                // 首先从env中根据array的名字取出其对应的value对象，以提取它的valType，从而确定构建数组的size
                ValuePtr con = interpreter.getFromEnv(this->arrayName->getName());
                size = interpreter.getVarTypeSize(con->getVarType());
                for (int i = 0; i < size; ++i) {
                    ConcreteNumValuePtr index = std::make_shared<ConcreteNumValue>("index", i);
                    // ValuePtr temp = std::make_shared<ArrayValue>(rightRand->getName(), rightRand->getArrayValue());
                    InternalBinValuePtr temp2bin = std::make_shared<InternalBinValue>(
                            this->arrayName->getName() + "_" + std::to_string(i), array, index,
                            ASTNode::Operator::SYMBOLINDEX);
                    //interpreter.addToSequence(temp2bin);
                    // 当array为concreteNumValue时, 被访问的每个元素的类型被设置为uint1
                    temp2bin->setVarType(ASTNode::uint1);
                    res.push_back(temp2bin);
                }
            }
            // 当array为internalUnvalue, touint时, 需要构建一个数组类型，其中每个元素都是一个internalBinValue，用SYMBOLINDEX连接，而被取值的对象是一个被touint连接的uints
            else if (array->getValueType() == ValueType::VTInternalUnValue) {
                // 首先从env中根据array的名字取出其对应的value对象，以提取它的valType，从而确定构建数组的size
                ValuePtr con = interpreter.getFromEnv(this->arrayName->getName());
                size = interpreter.getVarTypeSize(con->getVarType());
                for (int i = 0; i < size; ++i) {
                    ConcreteNumValuePtr index = std::make_shared<ConcreteNumValue>("index", i);
                    // ValuePtr temp = std::make_shared<ArrayValue>(rightRand->getName(), rightRand->getArrayValue());
                    InternalBinValuePtr temp2bin = std::make_shared<InternalBinValue>(
                            this->arrayName->getName() + "_" + std::to_string(i), array, index,
                            ASTNode::Operator::SYMBOLINDEX);
                    //interpreter.addToSequence(temp2bin);
                    // 当array为concreteNumValue时, 被访问的每个元素的类型被设置为uint1
                    temp2bin->setVarType(ASTNode::uint1);
                    res.push_back(temp2bin);
                }
            }
            // 对某个输入参数取位
            else if (array->getValueType() == ValueType::VTParameterValue) {
                // 首先从env中根据array的名字取出其对应的value对象，以提取它的valType，从而确定构建数组的size
                ValuePtr con = interpreter.getFromEnv(this->arrayName->getName());
                size = interpreter.getVarTypeSize(con->getVarType());
                for (int i = 0; i < size; ++i) {
                    ConcreteNumValuePtr index = std::make_shared<ConcreteNumValue>("index", i);
                    // ValuePtr temp = std::make_shared<ArrayValue>(rightRand->getName(), rightRand->getArrayValue());
                    InternalBinValuePtr temp2bin = std::make_shared<InternalBinValue>(
                            this->arrayName->getName() + "_" + std::to_string(i), array, index,
                            ASTNode::Operator::SYMBOLINDEX);
                    //interpreter.addToSequence(temp2bin);
                    // 当array为concreteNumValue时, 被访问的每个元素的类型被设置为uint1
                    temp2bin->setVarType(ASTNode::uint1);
                    res.push_back(temp2bin);
                }
            }
            // 不符合上述所有情况时报错，说明有某些情况目前还没有覆盖到的
            else {
                assert(false);
            }
        }

        ValuePtr temp1 = std::make_shared<ArrayValue>(this->getArrayName()->getName(), res);
        arrayV = dynamic_cast<ArrayValue *>(temp1.get());
        // 当转化为数组以后，还需要更新数组至env，因为后续计算index会在env中查询数组的size
        //interpreter.addToEnv(this->getArrayName()->getName(), std::make_shared<ArrayValue>(this->getArrayName()->getName(), res));
        interpreter.addToEnv(this->getArrayName()->getName(), temp1);
        std::vector<int> resArraySize;
        resArraySize.push_back(res.size());
        interpreter.setArraySize(this->getArrayName()->getName(), resArraySize);
    }

//    vector<int> indexes = interpreter.computeIndex(name, this->dimons);

    /*
     * 在这里我们要计算index，在这里我们分为两种情况：
     * 1. index是包含输入参数的，此时index具体的值不能被计算出来，只能用symbol方式表示出来；
     * 2. index没有包含输入参数，index具体值可以被计算出，此时计算index的具体值又分为三种情况：
     *      1）当数组是一维的时候，index的值直接计算即可；
     *      2）当数组是二维的时候，index的值需要由两个值来决定，具体的，
     *         数组a中，a[i1][i2]的具体index值需要求解的是 i1 * rowSize + i2
     *      3）当数组是一维，取到数组某一个元素时，又对该元素的某位bit取值，如 rc[r-1][i]
     * */
    std::vector<ValuePtr> indexesSymbol = interpreter.computeIndexWithSymbol(name, this->dimons);

    bool indexHasSymbol = false;
    for(auto ele : indexesSymbol) {
        if(!ValueCommon::isNoParameter(ele)) {
            indexHasSymbol = true;
            interpreter.getEnv()[name]->set_symbol_value_f();
        }
    }


    if(!indexHasSymbol and !interpreter.getEnv()[name]->get_symbol_value_f()) {
        std::vector<int> indexes;
        for (auto ele : indexesSymbol) {
            // 如果ele本身是不可以evaluate的，那么就需要直接使用其表达式形式
            int res = ele->value_of(interpreter.getEnv());
            indexes.push_back(res);
        }

        if(indexes.size() == 1) {
            // 如果computeIndexWithSymbol时index是不可计算的，那么返回的index应该是一个空指针

            /*
             * 这里有一种情况会带来后续的问题，比如，当arrayV->getValueAt(indexes[0])是一个concreteNumValue时，会直接返回一个具体的数值，
             * 此时的返回值除了具体值以外是没有其他任何信息的，但是如果我们需要进一步对该具体的数值取某一位时，就不能继续往下进行了，
             * 因为我们还需要知道这个具体的数值在内存中存储时是按照多少位来存储的。
             * 所以，这里我们首先判断arrayV->getValueAt(indexes[0])是否是concreteNumValue，如果是，
             * 那我们构建一个Value对象，将该具体的存储进去，并且还需要根据数组arrayV的varType，来确定arrayV->getValueAt(indexes[0])的VarType
             * */
            if (arrayV->getValueAt(indexes[0])->getValueType() == ValueType::VTConcreteNumValue) {
                ValuePtr con = arrayV->getValueAt(indexes[0]);
                // 这里我们还将该具体值的value对象命名为 “所取自数组名+索引值”
                // 注释：2022.8.4改： 具体值的value对象命名为 “所取自数组名 + "_" + 索引值”
                con->setName(arrayV->getName() + "_" + std::to_string(indexes[0]));
                con->setVarType(arrayV->getVarType());
                return con;
            }
            // 如果不是具体的数值，目前的做法是直接返回该位的元素，此时返回的的应该是一个SYMBOLINDEX的形式或者和parameter相关的index形式
            else
                return arrayV->getValueAt(indexes[0]);
        } else {
            std::vector<ValuePtr> res;
            std::string returnName = name + "_" + std::to_string(indexes.at(0)) + "_" + std::to_string(indexes.at(indexes.size() - 1));
            // std::cout << "returnName : " << returnName << std::endl;
            for(auto ele : indexes)
                res.push_back(arrayV->getValueAt(ele));
            return std::make_shared<ArrayValue>(returnName, res);
        }
    } else {
        // 如果有symbol
        // 这里我们改一下，如果有symbol，我们将其命名位name+SYMBOLINDEX，从而不会覆盖env中原有的name标记的value
        if(indexesSymbol.size() == 1) {
            std::shared_ptr<ArrayValueIndex> res = std::make_shared<ArrayValueIndex>(name+"_symbol", array, indexesSymbol[0]);
            assert(res->getArrayValuePtr()->getValueType() == ValueType::VTArrayValue);
            res->setVarType(arrayV->getVarType());
            interpreter.addToEnv(name+"_symbol", res);
            return res;
        } else {
            std::vector<ValuePtr> res;
            std::string returnName = name + "symbol_symbol";
            for(auto ele : indexesSymbol) {
                std::shared_ptr<ArrayValueIndex> temp = std::make_shared<ArrayValueIndex>(name, array, ele);
                assert(temp->getArrayValuePtr()->getValueType() == ValueType::VTArrayValue);
                res.push_back(temp);
            }
            return std::make_shared<ArrayValue>(returnName, res);
        }

    }




    /**
    int index = 0;
    int dimons = this->dimons->size();
    for(int i = 0; i < dimons; i++) {
        auto arraySize = (this->dimons)->at(i);
        if(arraySize->getTypeName() == "NInteger") {
            NIntegerPtr ident = dynamic_pointer_cast<NInteger>(arraySize);
            if(i != dimons - 1)
                index += sizeVec[i + 1] * ident->getValue();
            else
                index += ident->getValue();
        } else if(arraySize->getTypeName() == "NIdentifier") {
            NIdentifierPtr ident = dynamic_pointer_cast<NIdentifier>(arraySize);
            if(i != dimons - 1)
                index += sizeVec[i + 1] * interpreter.value_of(ident->compute(interpreter));
            else
                index += interpreter.value_of(ident->compute(interpreter));;
        } else if(arraySize->getTypeName() == "NBinaryOperator") {
            int value = interpreter.value_of(arraySize->compute(interpreter));
            if(i != dimons - 1)
                index += sizeVec[i + 1] * value;
            else
                index += value;
        } else {
            assert(false);
        }
    }

    int number = 1;
    if(dimons < sizeVec.size()) {
        // 意味着要返回一个数组的值了，不是单个的值
        // from index to
        // 计算个数就是后面所有的乘起来
        for(int i = dimons; i < sizeVec.size(); i++) {
            number *= sizeVec.at(i);
        }
        cout << number << endl;
    }


    cout << "index: " << index << endl;
    ArrayValue *arrayV = dynamic_cast<ArrayValue *>(array.get());
    if(dimons == sizeVec.size()) {
        ValuePtr res = nullptr;
        if (arrayV) {
            res = arrayV->getValueAt(index);
        } else {
            assert(false);
        }
        return res;
    } else {
        vector<ValuePtr> resArray;
        for(int i = index; i < index + number; i++) {
            resArray.push_back(arrayV->getValueAt(i));
        }
        return make_shared<ArrayValue>(resArray);
    }
     **/

}


// 从环境中获取值，然后再赋值给另一个数组
ValuePtr ASTNode::NArrayAssignment::compute(Interpreter &interpreter) {
//    cout << "Generate array index assignment of " <<  this->arrayIndex->getArrayName()->getName() << endl;

    // debug
    /*if (this->arrayIndex->getArrayName()->getName() == "rtn")
        std::cout << "stop here" << std::endl;*/

    ValuePtr right = this->expression->compute(interpreter);
    std::string name = this->arrayIndex->getArrayName()->getName();
    ValuePtr array = interpreter.getFromEnv(name);
    ArrayValue* arrayV = dynamic_cast<ArrayValue*>(array.get());
    assert(arrayV);


    std::vector<int> indexes = interpreter.computeIndex(name, this->arrayIndex->dimons);

    // 获取index，修改对应index的值
//    int dimons = this->arrayIndex->dimons->size();
//    int index = -1;
//    for(int i = 0; i < dimons; i++) {
//        auto arraySize = this->arrayIndex->dimons->at(i);
//        if(arraySize->getTypeName() == "NInteger") {
//            NIntegerPtr ident = dynamic_pointer_cast<NInteger>(arraySize);
//            index = ident->getValue();
//        } else if(arraySize->getTypeName() == "NIdentifier") {
//            NIdentifierPtr ident = dynamic_pointer_cast<NIdentifier>(arraySize);
//            index = interpreter.value_of(ident->compute(interpreter));
//        } else {
//            assert(false);
//        }
//    }

    //对数组元素的覆盖也算吧
    std::vector<ValuePtr> res;
    if(indexes.size() == 1) {
        // 只有一个元素
        int index = indexes[0];
        right->setName(name + "_" + std::to_string(index));
        ValuePtr valuePtr = interpreter.getEnv()[this->arrayIndex->getArrayName()->getName()];
        ASTNode::Type type = valuePtr->getVarType();
        right->setVarType(type);
        for(int i = 0; i < arrayV->getArrayValue().size(); i++) {
            if(i == index) {
                res.push_back(right);

                // 如果需要考虑hd的情况
                if(Interpreter::isHD) {
                    ValuePtr oldValue = arrayV->getArrayValue()[i];
                    if(oldValue) {
                        // 重复赋值
                        ValuePtr hdValue = std::make_shared<InternalBinValue>("hd", right, oldValue, ASTNode::Operator::XOR);
                        right->addParents(hdValue);
                        oldValue->addParents(hdValue);
                        interpreter.addToHDSequence(hdValue);
                    }
                }
            }
            else {
                res.push_back(arrayV->getArrayValue()[i]);
            }
        }
    } else {
        // 是一个数组
        ArrayValue* rightArray = dynamic_cast<ArrayValue*>(right.get());
        assert(rightArray);
        assert(rightArray->getArrayValue().size() == indexes.size());
        for(int i = 0; i < arrayV->getArrayValue().size(); i++) {
            if(i == indexes[0]) {
                for(int j = 0; j < rightArray->getArrayValue().size(); j++) {
                    // 如果需要考虑hd的情况
                    if(Interpreter::isHD) {
                        ValuePtr oldValue = arrayV->getValueAt(j);
                        if(oldValue) {
                            // 重复赋值
                            ValuePtr hdValue = std::make_shared<InternalBinValue>("hd", rightArray->getValueAt(j), oldValue, ASTNode::Operator::XOR);
                            rightArray->getValueAt(j)->addParents(hdValue);
                            oldValue->addParents(hdValue);
                            interpreter.addToHDSequence(hdValue);
                        }
                    }
                    res.push_back(rightArray->getValueAt(j));
                    i++;
                }
                i = i - 1;
            } else
                res.push_back(arrayV->getArrayValue()[i]);
        }

    }

//    for(int i = 0; i < arrayV->getArrayValue().size(); i++) {
//        if(i == index)
//            res.push_back(right);
//        else
//            res.push_back(arrayV->getArrayValue()[i]);
//    }

    // 在这个里面，不知道res是一个数组的值还是一个单个值呢？
//    if(interpreter.isHD) {
//        // 如果是在HD model下，就要判断是不是重复赋值了
//        ValuePtr oldValue = interpreter.getFromEnv(this->LHS->getName());
//        if(oldValue) {
//            // 重复赋值
//            ValuePtr hdValue = make_shared<InternalBinValue>(expValue, oldValue, ASTNode::Operator::XOR);
//            interpreter.addToHDSequence(hdValue);
//        }
//    }

    // 这里好像只要是对数组中元素的赋值，都是返回的是
    std::string returnName = name + "_" + std::to_string(indexes[0]) + "_" + std::to_string(indexes[indexes.size() - 1]);
    ValuePtr valuePtr = std::make_shared<ArrayValue>(returnName, res);
    ValuePtr originArray = interpreter.getFromEnv(this->arrayIndex->getArrayName()->getName());
    valuePtr->setVarType(originArray->getVarType());
    interpreter.addToEnv(name, valuePtr);

    // std::cout << "NArrayAssignment returnName : " << returnName << std::endl;

    if(right->getValueType() == ValueType::VTConcreteNumValue) {
        // ConcreteNumValue不需要加入到sequence中去
    }else if(right->getValueType() == ValueType::VTInternalBinValue) {
        // 符合表达式也不用放进去了，因为在自身的时候已经放进去过了
//        cout << "added in computation: " << endl;

        // 当表达式是symbolindex时，还需要添加sequence，因为这个在生成时还没有添加进去
        InternalBinValue* internalBinValue = dynamic_cast<InternalBinValue*>(right.get());
        if (internalBinValue->getOp() == ASTNode::Operator::SYMBOLINDEX) {
           /* ValuePtr valuePtr = interpreter.getEnv()[this->arrayIndex->getArrayName()->getName()];
            ASTNode::Type type = valuePtr->getVarType();
            right->setVarType(type);*/
           // 如果已经别加入到sequence中，就不需要重复添加了。
           // 这里可能重复添加是因为分别处理的 uints 和 uint1 数组两种情况。
           std::vector<ValuePtr> sequenceTemp = interpreter.getSequence();
           bool flag = false;
           for (auto eleTemp : sequenceTemp) {
               if (eleTemp->getName() == right->getName()) {
                   flag = true;
                   break;
               }
           }
            if (!flag)
               interpreter.addToSequence(right);
        }
        else if (internalBinValue->getOp() == ASTNode::Operator::BOXOP) {
            right->setName(name + "_" + std::to_string(indexes[0]));
            interpreter.addToSequence(right);
        }
        else if (internalBinValue->getOp() == ASTNode::Operator::XOR) {

        }
        else if (internalBinValue->getOp() == ASTNode::Operator::ADD) {

        }
        else if (internalBinValue->getOp() == ASTNode::Operator::AND) {

        }
        else {
            std::cout << "stop !" << std::endl;
            assert(false);
        }
    }
    else if(right->getValueType() == ValueType::VTInternalUnValue) {
//        cout << "added in computation: " << endl;
    } else {
        interpreter.addToSequence(right);
    }

    return array;
}

ValuePtr ASTNode::NArrayInitialization::compute(Interpreter &interpreter) {
    // 1. 先计算声明，在环境中开辟一席之地
    // 2. 把值读出来，每个值计算value，并放到vector中
    // 3. 重新放到env中

    // debug
    /*if (declaration->getId()->getName() == "rtn")
        std::cout << "stop here !" << std::endl;*/


    this->declaration->compute(interpreter);

    NExpressionListPtr nExpressionListPtr = this->expressionList;
    std::vector<ValuePtr> result;
    for(int i = 0; i < nExpressionListPtr->size(); i++) {
        ValuePtr valuePtr = nExpressionListPtr->at(i)->compute(interpreter);
        valuePtr->setVarType(interpreter.VarTypeTrans(this->declaration->getMyType()));
        result.push_back(valuePtr);
    }

    std::string arrayName = this->declaration->getId()->getName();
    ValuePtr res = std::make_shared<ArrayValue>(arrayName, result);
    res->setVarType(interpreter.VarTypeTrans(this->declaration->getMyType()));
    interpreter.addToEnv(arrayName, res);

    return nullptr;
}


ValuePtr ASTNode::NSboxInitialization::compute(Interpreter &interpreter) {
    /*std::cout << "Compute: Generating Sbox declaration : \n"
              << "declaration type : " << this->declaration->getMyType()
              << "\ndeclaration name : " << this->declaration->getId()->getName() << std::endl;*/

    // 用于存储sbox的信息
    std::vector<int> sboxEles;

    this->declaration->compute(interpreter);
    NIntegerListPtr integerListPtr = this->nIntegerListPtr;
    std::vector<ValuePtr> result;
    for(int i = 0; i < integerListPtr->size(); i++) {
        ValuePtr valuePtr = integerListPtr->at(i)->compute(interpreter);
        valuePtr->setVarType(interpreter.VarTypeTrans(this->declaration->getMyType()));
        result.push_back(valuePtr);

        ConcreteNumValue* concreteNumValue = dynamic_cast<ConcreteNumValue*>(valuePtr.get());
        sboxEles.push_back(concreteNumValue->getNumer());
    }
    std::string arrayName = this->declaration->getId()->getName();
    int rowSize = integerListPtr->size();
    ValuePtr sbox = std::make_shared<BoxValue>(arrayName, "sbox", rowSize, result);
    sbox->setVarType(interpreter.VarTypeTrans(this->declaration->getMyType()));
    interpreter.addToEnv(arrayName, sbox);

    // 存储sbox
    // 存储每个box时，需要将该box的类型也存储，这样可以在后续处理时选择合适的对应方式
    allBox["sbox" + this->declaration->getId()->getName()] = sboxEles;

    return nullptr;
}

ValuePtr ASTNode::NPboxInitialization::compute(Interpreter &interpreter) {
    // 用于存储pbox的信息
    std::vector<int> pboxEles;

    this->declaration->compute(interpreter);
    NIntegerListPtr integerListPtr = this->nIntegerListPtr;
    std::vector<ValuePtr> result;
    for(int i = 0; i < integerListPtr->size(); i++) {
        ValuePtr valuePtr = integerListPtr->at(i)->compute(interpreter);
        valuePtr->setVarType(interpreter.VarTypeTrans(this->declaration->getMyType()));
        result.push_back(valuePtr);

        ConcreteNumValue* concreteNumValue = dynamic_cast<ConcreteNumValue*>(valuePtr.get());
        pboxEles.push_back(concreteNumValue->getNumer());
    }
    std::string arrayName = this->declaration->getId()->getName();
    int rowSize = integerListPtr->size();
    ValuePtr pbox = std::make_shared<BoxValue>(arrayName, "pbox", rowSize, result);
    pbox->setVarType(interpreter.VarTypeTrans(this->declaration->getMyType()));
    interpreter.addToEnv(arrayName, pbox);

    // 存储pbox
    allBox["pbox" + this->declaration->getId()->getName()] = pboxEles;

    return nullptr;
}

ValuePtr ASTNode::NPboxmInitialization::compute(Interpreter &interpreter) {
    this->declaration->compute(interpreter);
    NIntegerListListPtr integerListListPtr = this->nIntegerListListPtr;
    std::vector<ValuePtr> result;
    for(int i = 0; i < nIntegerListListPtr->size(); i++) {
        for (int j = 0; j < nIntegerListListPtr->at(i)->size(); ++j) {
            ValuePtr valuePtr = nIntegerListListPtr->at(i)->at(j)->compute(interpreter);
            valuePtr->setVarType(interpreter.VarTypeTrans(this->declaration->getMyType()));
            result.push_back(valuePtr);
        }
    }
    std::string arrayName = this->declaration->getId()->getName();
    // 二维的pboxm的rowSize定义为每一行的size，这里取nIntegerListListPtr的第一个元素的size
    int rowSize = nIntegerListListPtr->at(0)->size();
    ValuePtr pboxm = std::make_shared<BoxValue>(arrayName, "pboxm", rowSize, result);
    pboxm->setVarType(interpreter.VarTypeTrans(this->declaration->getMyType()));
    interpreter.addToEnv(arrayName, pboxm);
    return nullptr;
}

ValuePtr ASTNode::NFfmInitialization::compute(Interpreter &interpreter) {
    // 在ffm的declaration进行compute时，先将对应的pboxm取出来。因为compute会计算出相同ident的M，覆盖pboxm
    ValuePtr M = interpreter.getFromEnv(this->declaration->getId()->getName());
    this->declaration->compute(interpreter);
    // 在compute结束之后，重新添加之前的pboxm至env
    interpreter.addToEnv(this->declaration->getId()->getName(), M);

    NIntegerListListPtr integerListListPtr = this->nIntegerListListPtr;
    std::vector<ValuePtr> result;
    for(int i = 0; i < nIntegerListListPtr->size(); i++) {
        for (int j = 0; j < nIntegerListListPtr->at(i)->size(); ++j) {
            ValuePtr valuePtr = nIntegerListListPtr->at(i)->at(j)->compute(interpreter);
            result.push_back(valuePtr);
        }
    }
    std::string arrayName = this->declaration->getId()->getName();
    int rowSize = nIntegerListListPtr->size();
    ValuePtr ffm = std::make_shared<BoxValue>(arrayName, "ffm", rowSize, result);
    ffm->setVarType(interpreter.VarTypeTrans(this->declaration->getMyType()));
    interpreter.addToEnv(arrayName + "_ffm", ffm);
    return nullptr;
}

/*ValuePtr ASTNode::NForStatement::compute(Interpreter &interpreter) {
    ValuePtr intiValue = this->init->compute(interpreter);
    ValuePtr cond = this->condition->compute(interpreter);
    bool cont = interpreter.value_of(cond);
    while(cont) {
        this->block->compute(interpreter);
        ValuePtr inc = this->increase->compute(interpreter);
        cond = this->condition->compute(interpreter);
        cont = interpreter.value_of(cond);
    }
    return nullptr;
}*/


// compute function for for-statement : for(i from n1 to n2) {S}
ValuePtr ASTNode::NForStatement::compute(Interpreter &interpreter) {
    ValuePtr intiValue = this->from->compute(interpreter);
    // 将初始时i以及目前i的值添加至env，以便在block进行处理时，可以根据i的值来计算index的值。
    interpreter.addToEnv(this->ident->getName(), intiValue);

    int lowerBound = this->from->getValue();
    int upperBound = this->to->getValue();
    bool cont = lowerBound <= upperBound;
    while(cont) {
        this->block->compute(interpreter);
        lowerBound++;
        // 将i以及目前i的值添加至env，以便在block进行处理时，可以根据i的值来计算index的值。
        interpreter.addToEnv(this->ident->getName(), std::make_shared<ConcreteNumValue>("", lowerBound));

        // for debug
        //std::cout << "for statement debug : " << std::endl;
        //std::cout << "this->ident->getName() : " << this->ident->getName() << std::endl;
        //std::cout << "std::make_shared<ConcreteNumValue>(\"\", lowerBound) : " << std::make_shared<ConcreteNumValue>("", lowerBound)->toString() << std::endl;

        cont = lowerBound <= upperBound;
    }
    return nullptr;
}


ValuePtr ASTNode::NInteger::compute(Interpreter &interpreter) {
    // TYPING SYSTEM : 所有的整数需要大于等于0
    // 后面考虑使用yylineno打印出错的行数
    // assert(this->value < 0);
    if (this->value < 0) {
        std::cout << "TYPE ERROR : Integer value less than 0" << std::endl;
        abort();
    }

    ValuePtr valuePtr = std::make_shared<ConcreteNumValue>("", this->value);
    return std::make_shared<ConcreteNumValue>("", this->value);
}


ValuePtr ASTNode::NFunctionCall::compute(Interpreter &interpreter) {
//    cout << "Generate function call of " << this->ident->getName() << endl;

    // find function name
    std::string functionName = this->ident->getName();

    // find real parameter
    // 是数组就直接放数组就好了
    std::vector<ValuePtr> realParameters;
    for (auto arg : *(this->arguments)) {
        ValuePtr argPtr = arg->compute(interpreter);
        realParameters.push_back(argPtr);
    }

    // find function body
    ValuePtr proc = interpreter.getProc(functionName);
    ProcedurePtr procedurePtr = nullptr;
    if(ProcValue* procValue = dynamic_cast<ProcValue*>(proc.get())) {
        procedurePtr = procValue->getProcedurePtr();
    } else {
        assert(false);
    }


    // 所以这里需要考虑的是对于一个函数调用，其返回值应该是什么？
    // 我觉得，如果返回是一个具体的值，就是那个值，如果是一个vector，就应该是一个vector

    std::vector<int> callsites;
    callsites.push_back(this->callSite);
    std::string callsitesString = "";
    for (auto ele : callsites) callsitesString += "@" + std::to_string(ele);
    ProcCallValuePtr procCallValuePtr = std::make_shared<ProcCallValue>(procedurePtr->getProcName() + callsitesString,
                                                                   procedurePtr, realParameters, callsites);

    const ValuePtr& returnValue = procedurePtr->getReturns();
    ArrayValue* arrayValue = dynamic_cast<ArrayValue*>(returnValue.get());
    ValuePtr res;
    if(arrayValue) {
        // 如果返回值是一个数组
        std::vector<ValuePtr> returnArray;

        for (int i = 0; i < arrayValue->getArrayValue().size(); i++) {
            ValuePtr value = std::make_shared<ProcCallValueIndex>(procCallValuePtr->getName() + "_" + std::to_string(i),
                                                             procCallValuePtr, i);
            value->setVarType(proc->getVarType());
            returnArray.push_back(value);
        }

        res = std::make_shared<ArrayValue>(procCallValuePtr->getName() + "_ret", returnArray);
        res->setVarType(proc->getVarType());

        // ProcCallValue的parent是各个返回值
        for (auto ele : returnArray) {
            procCallValuePtr->addParents(ele);
        }
    } else {
        // 如果返回值是单个值
        res = std::make_shared<ProcCallValueIndex>(procCallValuePtr->getName() + "_" + std::to_string(0), procCallValuePtr, 0);
        res->setVarType(proc->getVarType());
        procCallValuePtr->addParents(res);
    }

    // 实际参数的parent是ProcCallValue
    for (auto ele : realParameters) {
        if (ArrayValue *array = dynamic_cast<ArrayValue *>(ele.get())) {
            for (auto ele1 : array->getArrayValue()) {
//                assert(ele1);
                if(ele1)
                    ele1->addParents(procCallValuePtr);
            }
        } else {
            ele->addParents(procCallValuePtr);
        }
    }

    return res;
}


// View的返回对象是数组
ValuePtr ASTNode::NViewArray::compute(Interpreter &interpreter) {
    // 从当前的Env中找到View操作的数组对象
    std::map<std::string, ValuePtr> temptopTnv = interpreter.getEnv();
    for (auto ele : temptopTnv) {
        if (ele.first == this->arrayName->getName()) {

        }
    }
    ValuePtr array = interpreter.getEnv()[this->arrayName->getName()];
    ArrayValue *arrayV = dynamic_cast<ArrayValue *>(array.get());
    assert(arrayV);

    int lowerBound = from->compute(interpreter)->value_of(interpreter.getEnv());
    int upperBound = to->compute(interpreter)->value_of(interpreter.getEnv());
    std::vector<ValuePtr> resArray;
    for (int i = lowerBound; i <= upperBound; ++i) {
        resArray.push_back(arrayV->getValueAt(i));
    }
    //ArrayValue res(this->arrayName->getName() + "_view", resArray);
    return std::make_shared<ArrayValue>(this->arrayName->getName() + "_view", resArray);
}


// touint的返回对象是一个变量
ValuePtr ASTNode::NToUint::compute(Interpreter &interpreter) {
    std::string name;
    //返回一个VTInternalBinValue类型，由操作符TOUINT链接标识符和数组
    std::vector<ValuePtr> expListTrans;
    // 当NToUint对象的实例由expressionListPtr初始化
    int size = 0;
    if (this->expressionListPtr != nullptr) {
        for (auto exp : *expressionListPtr) {
            ValuePtr expValue = exp->compute(interpreter);
            name = expValue->getName();
            expListTrans.push_back(expValue);
            size++;
        }
    }
    // 当NToUint对象的实例由arrayName初始化
    else if (this->arrayName != nullptr) {

    }

    ValuePtr arrayValue = std::make_shared<ArrayValue>(name + "touint", expListTrans);
    ValuePtr rtn = std::make_shared<InternalUnValue>(name + "tounint", arrayValue, ASTNode::Operator::TOUINT);
    rtn->setVarType(interpreter.int2Type(size));
    return rtn;
}


ValuePtr ASTNode::NBoxOperation::compute(Interpreter &interpreter) {
    ValuePtr L = boxname->compute(interpreter);
    ValuePtr R = expressionPtr->compute(interpreter);
    ValuePtr res = std::make_shared<InternalBinValue>("", L, R, ASTNode::Operator::BOXOP);
    res->set_symbol_value_f();
    res->setVarType(R->getVarType());
    //interpreter.addToSequence(res);
    L->addParents(res);
    R->addParents(res);
    return res;
}

ValuePtr ASTNode::NCipherNameDeclaration::compute(Interpreter &interpreter) {
    return nullptr;
}