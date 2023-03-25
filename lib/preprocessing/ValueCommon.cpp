//
// Created by Septi on 6/16/2022.
//

#include "ValueCommon.h"

// 这里我注释了所有Value中没有的Value Class子类
namespace ValueCommon {

    bool containsProcCall(ValuePtr valuePtr, ValuePtr& res) {
        bool flag = false;
        std::set<ValuePtr> visited;
        containsProcCallHelper(valuePtr, flag, res, visited);
        return flag;
    }

    void containsProcCallHelper(ValuePtr valuePtr, bool& flag, ValuePtr& res, std::set<ValuePtr>& visited) {
        if(flag)
            return;
        /*if (valuePtr->getValueType() == ValueType::VTRandomVable) {
            return;
        } else if (valuePtr->getValueType() == ValueType::VTPublicValue) {
            return;
        } else if (valuePtr->getValueType() == ValueType::VTPrivateValue) {
            return;
        }*/
        if (valuePtr->getValueType() == ValueType::VTParameterValue) {
            return;
        }
        else if(valuePtr->getValueType() == ValueType::VTProcCallValueIndex) {
            res = valuePtr;
            flag = true;
            return;
        } else if(InternalBinValue* value = dynamic_cast<InternalBinValue*>(valuePtr.get())) {
            if(visited.find(valuePtr) == visited.end()) {
                containsProcCallHelper(value->getLeft(), flag, res, visited);
                if (!flag)
                    containsProcCallHelper(value->getRight(), flag, res, visited);
                visited.insert(valuePtr);
            }
        } else if(InternalUnValue* value = dynamic_cast<InternalUnValue*>(valuePtr.get())) {
            if(visited.find(valuePtr) == visited.end()) {
                containsProcCallHelper(value->getRand(), flag, res, visited);
                visited.insert(valuePtr);
            }
        } else if(ArrayValue* value = dynamic_cast<ArrayValue*>(valuePtr.get())) {
            for(auto ele : value->getArrayValue()) {
                if (!flag && visited.find(ele) == visited.end()) {
                    containsProcCallHelper(ele, flag, res, visited);
                    visited.insert(ele);
                }
            }
        } else {
            return;
        }
    }


    bool isFullComputation(ValuePtr valuePtr) {
        return isNoParameter(valuePtr) && isNoProcCall(valuePtr) && isNoArrayIndex(valuePtr);
    }

    bool isNoParameter(ValuePtr valuePtr) {
        std::set<ValuePtr> visited;
        bool noPara = true;
        isNoParameterHelper(std::move(valuePtr), noPara, visited);
        return noPara;
    }

    void isNoParameterHelper(ValuePtr valuePtr, bool& noPara, std::set<ValuePtr>& visited) {
        if(!noPara)
            return;
        /*if(valuePtr->getValueType() == ValueType::VTRandomVable) {
            return;
        }
        else if(valuePtr->getValueType() == ValueType::VTPrivateValue) {
            return;
        }
        else if(valuePtr->getValueType() == ValueType::VTPublicValue) {
            return;
        }*/
        if(valuePtr->getValueType() == ValueType::VTParameterValue) {
            noPara = false;
            return;
        } else if(valuePtr->getValueType() == ValueType::VTConstantValue) {
            noPara = false;
            return;
        }
        else if(valuePtr->getValueType() == ValueType::VTProcCallValueIndex) {
            return;
        } else if(InternalBinValue* value = dynamic_cast<InternalBinValue*>(valuePtr.get())) {
            if(visited.find(valuePtr) == visited.end()) {
                isNoParameterHelper(value->getLeft(), noPara, visited);
                if (noPara)
                    isNoParameterHelper(value->getRight(), noPara, visited);
                visited.insert(valuePtr);
            }
        } else if(InternalUnValue* value = dynamic_cast<InternalUnValue*>(valuePtr.get())) {
            if(visited.find(valuePtr) == visited.end()) {
                isNoParameterHelper(value->getRand(), noPara, visited);
                visited.insert(valuePtr);
            }
        } else if(ArrayValue* value = dynamic_cast<ArrayValue*>(valuePtr.get())) {
            for(auto ele : value->getArrayValue())
                if (noPara && visited.find(ele) == visited.end()) {
                    isNoParameterHelper(ele, noPara, visited);
                    visited.insert(ele);
                }
        }
        else if (ArrayValueIndex* value = dynamic_cast<ArrayValueIndex*>(valuePtr.get())) {
            // 这里我们新添加一个判断条件，对于有的数组而言，其数组本身是常数数组，但是需要访问的参数位置为symbol类型，此时需要判断一下
            if (value->getSymbolIndex() != nullptr)
                noPara = false;
            return;
        }
        else {
            return;
        }
    }

    bool isNoRandoms(ValuePtr valuePtr) {
        std::set<ValuePtr> visited;
        bool noRandoms = true;
        isNoRandomsHelper(valuePtr, noRandoms, visited);
        return noRandoms;
    }
    void isNoRandomsHelper(ValuePtr valuePtr, bool& noRands, std::set<ValuePtr>& visited) {
        if(!noRands)
            return;
        /*if(valuePtr->getValueType() == ValueType::VTRandomVable) {
            noRands = false;
            return;
        }
        else if(valuePtr->getValueType() == ValueType::VTPrivateValue) {
            return;
        }
        else if(valuePtr->getValueType() == ValueType::VTPublicValue) {
            return;
        }*/
        if(valuePtr->getValueType() == ValueType::VTParameterValue) {
            return;
        } else if(valuePtr->getValueType() == ValueType::VTConstantValue) {
            return;
        }
        else if(valuePtr->getValueType() == ValueType::VTProcCallValueIndex) {
            return;
        } else if(InternalBinValue* value = dynamic_cast<InternalBinValue*>(valuePtr.get())) {
            if(visited.find(valuePtr) == visited.end()) {
                isNoRandomsHelper(value->getLeft(), noRands, visited);
                if (noRands) isNoRandomsHelper(value->getRight(), noRands, visited);
                visited.insert(valuePtr);
            }
        } else if(InternalUnValue* value = dynamic_cast<InternalUnValue*>(valuePtr.get())) {
            if(visited.find(valuePtr) == visited.end()) {
                isNoRandomsHelper(value->getRand(), noRands, visited);
                visited.insert(valuePtr);
            }
        } else if(ArrayValue* value = dynamic_cast<ArrayValue*>(valuePtr.get())) {
            for(auto ele : value->getArrayValue())
                if (noRands && visited.find(ele) == visited.end()) {
                    isNoRandomsHelper(ele, noRands, visited);
                    visited.insert(ele);
                }
        } else {
            return;
        }
    }

    bool isNoArrayIndex(ValuePtr valuePtr) {
        std::set<ValuePtr> visited;
        bool noArrayIndex = true;
        isNoArrayIndexHelper(valuePtr, noArrayIndex, visited);
        return noArrayIndex;
    }
    void isNoArrayIndexHelper(ValuePtr valuePtr, bool& noArrayIndex, std::set<ValuePtr>& visited) {
        if(!noArrayIndex)
            return;
        /*if(valuePtr->getValueType() == ValueType::VTRandomVable) {
            return;
        }
        else if(valuePtr->getValueType() == ValueType::VTPrivateValue) {
            return;
        }
        else if(valuePtr->getValueType() == ValueType::VTPublicValue) {
            return;
        }*/
        if(valuePtr->getValueType() == ValueType::VTParameterValue) {
            return;
        }
        else if(valuePtr->getValueType() == ValueType::VTArrayValueIndex) {
            noArrayIndex = false;
            return;
        }
        else if(valuePtr->getValueType() == ValueType::VTProcCallValueIndex) {
            return;
        } else if(InternalBinValue* value = dynamic_cast<InternalBinValue*>(valuePtr.get())) {
            if(visited.find(valuePtr) == visited.end()) {
                isNoArrayIndexHelper(value->getLeft(), noArrayIndex, visited);
                if (noArrayIndex) isNoArrayIndexHelper(value->getRight(), noArrayIndex, visited);
                visited.insert(valuePtr);
            }
        } else if(InternalUnValue* value = dynamic_cast<InternalUnValue*>(valuePtr.get())) {
            if(visited.find(valuePtr) == visited.end()) {
                isNoArrayIndexHelper(value->getRand(), noArrayIndex, visited);
                visited.insert(valuePtr);
            }
        } else if(ArrayValue* value = dynamic_cast<ArrayValue*>(valuePtr.get())) {
            for(auto ele : value->getArrayValue())
                if (noArrayIndex && visited.find(ele) == visited.end()) {
                    isNoArrayIndexHelper(ele, noArrayIndex, visited);
                    visited.insert(ele);
                }
        } else {
            return;
        }
    }

    bool isNoProcCall(ValuePtr valuePtr) {
        std::set<ValuePtr> visited;
        bool noProcCall = true;
        isNoProcCallHelper(valuePtr, noProcCall, visited);
        return noProcCall;
    }
    void isNoProcCallHelper(ValuePtr valuePtr, bool& noProcCall, std::set<ValuePtr>& visited) {
        if(!noProcCall)
            return;
        /*if(valuePtr->getValueType() == ValueType::VTRandomVable) {
            return;
        }
        else if(valuePtr->getValueType() == ValueType::VTPrivateValue) {
            return;
        }
        else if(valuePtr->getValueType() == ValueType::VTPublicValue) {
            return;
        }*/
        if(valuePtr->getValueType() == ValueType::VTParameterValue) {
            return;
        }
        else if(valuePtr->getValueType() == ValueType::VTProcCallValueIndex) {
            noProcCall = false;
            return;
        } else if(InternalBinValue* value = dynamic_cast<InternalBinValue*>(valuePtr.get())) {
            if(visited.find(valuePtr) == visited.end()) {
                isNoProcCallHelper(value->getLeft(), noProcCall, visited);
                if (noProcCall) isNoProcCallHelper(value->getRight(), noProcCall, visited);
                visited.insert(valuePtr);
            }
        } else if(InternalUnValue* value = dynamic_cast<InternalUnValue*>(valuePtr.get())) {
            if(visited.find(valuePtr) == visited.end()) {
                isNoProcCallHelper(value->getRand(), noProcCall, visited);
                visited.insert(valuePtr);
            }
        } else if(ArrayValue* value = dynamic_cast<ArrayValue*>(valuePtr.get())) {
            for(auto ele : value->getArrayValue())
                if (noProcCall && visited.find(ele) == visited.end()) {
                    isNoProcCallHelper(ele, noProcCall, visited);
                    visited.insert(ele);
                }
        } else {
            return;
        }
    }

    // 对表达式中能计算出具体值的子表达式，就计算出来，替换原来的表达式
    // 说白了，就是找到左孩子和右孩子都是具体值的点，计算出来并替换，找到左孩子是具体值且是单元操作符的点计算出来并替换
    ValuePtr compactExpression(ValuePtr valuePtr) {
        /*if(valuePtr->getValueType() == ValueType::VTRandomVable) {
            return valuePtr;
        }
        else if(valuePtr->getValueType() == ValueType::VTPrivateValue) {
            return valuePtr;
        }
        else if(valuePtr->getValueType() == ValueType::VTPublicValue) {
            return valuePtr;
        }*/
        if(valuePtr->getValueType() == ValueType::VTConcreteNumValue) {
            return valuePtr;
        }
        if(InternalBinValue* value = dynamic_cast<InternalBinValue*>(valuePtr.get())) {
            ValuePtr compactedLeft = compactExpression(value->getLeft());
            ValuePtr compactedRight = compactExpression(value->getRight());
            ValuePtr temp = std::make_shared<InternalBinValue>(value->getName(), compactedLeft, compactedRight, value->getOp());
            // 这里就要分很多种情况来处理了
            if(compactedLeft->getValueType() == ValueType::VTConcreteNumValue &&
               compactedRight->getValueType() == ValueType::VTConcreteNumValue) {
                std::map<std::string, ValuePtr> env;
                return std::make_shared<ConcreteNumValue>(value->getName(), temp->value_of(env));
            } else if(compactedLeft->getValueType() == ValueType::VTConcreteNumValue){
                ConcreteNumValue* leftValue = dynamic_cast<ConcreteNumValue*>(compactedLeft.get());

                if(leftValue->getNumer() == 0) {
                    if(value->getOp() == ASTNode::Operator::XOR) {
                        return compactedRight;
                    } else if(value->getOp() == ASTNode::Operator::AND) {
                        return compactedLeft;
                    } else if(value->getOp() == ASTNode::Operator::OR) {
                        return compactedRight;
                    } else {
                        return temp;
                    }
                } else if(leftValue->getNumer() == 1) {
                    if(value->getOp() == ASTNode::Operator::XOR) {
                        return std::make_shared<InternalUnValue>(value->getName(), compactedRight, ASTNode::Operator::NOT);
                    } else if(value->getOp() == ASTNode::Operator::AND) {
                        return compactedRight;
                    } else if(value->getOp() == ASTNode::Operator::OR) {
                        return compactedLeft;
                    } else {
                        return temp;
                    }
                } else {
                    return temp;
                }
            } else if(compactedRight->getValueType() == ValueType::VTConcreteNumValue){
                ConcreteNumValue* rightValue = dynamic_cast<ConcreteNumValue*>(compactedRight.get());
                if(rightValue->getNumer() == 0) {
                    if(value->getOp() == ASTNode::Operator::XOR) {
                        return compactedLeft;
                    } else if(value->getOp() == ASTNode::Operator::AND) {
                        return compactedRight;
                    } else if(value->getOp() == ASTNode::Operator::OR) {
                        return compactedLeft;
                    } else {
                        return temp;
                    }
                } else if(rightValue->getNumer() == 1) {
                    if(value->getOp() == ASTNode::Operator::XOR) {
                        return std::make_shared<InternalUnValue>(value->getName(), compactedLeft, ASTNode::Operator::NOT);
                    } else if(value->getOp() == ASTNode::Operator::AND) {
                        return compactedLeft;
                    } else if(value->getOp() == ASTNode::Operator::OR) {
                        return compactedRight;
                    } else {
                        return temp;
                    }
                } else {
                    return temp;
                }
            } else {
                return temp;
            }
        }
        if(InternalUnValue* value = dynamic_cast<InternalUnValue*>(valuePtr.get())) {
            ValuePtr compactedRand = compactExpression(value->getRand());
            ValuePtr temp = std::make_shared<InternalUnValue>(value->getName(), compactedRand, value->getOp());
            if(compactedRand->getValueType() == ValueType::VTConcreteNumValue ) { // && value->getOp() != ASTNode::Operator::POL) {
                std::map<std::string, ValuePtr> env;
                return std::make_shared<ConcreteNumValue>(value->getName(), temp->value_of(env));
            } else {
                return temp;
            }
        }
        assert(false);
    }

    // 判断是否有和target一样的函数调用在valuePtr中
    void isNoSameFunctionCall(ValuePtr valuePtr, ValuePtr target, bool& noSameFunctionCall, std::set<ValuePtr>& visited) {
        if(!noSameFunctionCall)
            return;
        /*if(valuePtr->getValueType() == ValueType::VTRandomVable) {
            return;
        }
        else if(valuePtr->getValueType() == ValueType::VTPrivateValue) {
            return;
        }
        else if(valuePtr->getValueType() == ValueType::VTPublicValue) {
            return;
        }*/
        if(valuePtr->getValueType() == ValueType::VTParameterValue) {
            return;
        }
        else if(valuePtr->getValueType() == ValueType::VTProcCallValueIndex) {
            ProcCallValueIndex* value = dynamic_cast<ProcCallValueIndex*>(valuePtr.get());
            if(value->getProcCallValuePtr() == target) {
                noSameFunctionCall = false;
                return;
            }
            ProcCallValue* procCallValuePtr = dynamic_cast<ProcCallValue*>(value->getProcCallValuePtr().get());
            for(auto ele : procCallValuePtr->getArguments()) {
                if(noSameFunctionCall && visited.find(ele) == visited.end()) {
                    isNoSameFunctionCall(ele, target, noSameFunctionCall, visited);
                    visited.insert(ele);
                }
            }
            visited.insert(valuePtr);
            return;
        } else if(InternalBinValue* value = dynamic_cast<InternalBinValue*>(valuePtr.get())) {
            if(visited.find(valuePtr) == visited.end()) {
                isNoSameFunctionCall(value->getLeft(), target, noSameFunctionCall, visited);
                if (noSameFunctionCall) isNoSameFunctionCall(value->getRight(), target, noSameFunctionCall, visited);
                visited.insert(valuePtr);
            }
        } else if(InternalUnValue* value = dynamic_cast<InternalUnValue*>(valuePtr.get())) {
            if(visited.find(valuePtr) == visited.end()) {
                isNoSameFunctionCall(value->getRand(), target, noSameFunctionCall, visited);
                visited.insert(valuePtr);
            }
        } else if(ArrayValue* value = dynamic_cast<ArrayValue*>(valuePtr.get())) {
            for(auto ele : value->getArrayValue())
                if (noSameFunctionCall && visited.find(ele) == visited.end()) {
                    isNoSameFunctionCall(ele, target, noSameFunctionCall, visited);
                    visited.insert(ele);
                }
        } else {
            return;
        }
    }

    void getProcCallNPara(ValuePtr valuePtr, std::set<ValuePtr>& visited, std::set<ProcedurePtr>& procs, std::set<ValuePtr>& parameters) {
        /*if(valuePtr->getValueType() == ValueType ::VTRandomVable) {
            return;
        }
        else if(valuePtr->getValueType() == ValueType ::VTPrivateValue ) {
            return;
        }
        else if(valuePtr->getValueType() == ValueType ::VTPublicValue ) {
            return;
        }*/
        if(valuePtr->getValueType() == ValueType ::VTParameterValue ) {
            parameters.insert(valuePtr);
            return;
        }
        else if(ProcCallValueIndex* value = dynamic_cast<ProcCallValueIndex*>(valuePtr.get())) {
            ProcCallValue* procCallValuePtr = dynamic_cast<ProcCallValue*>(value->getProcCallValuePtr().get());
            procs.insert(procCallValuePtr->getProcedurePtr());
            return;
        } else if(InternalBinValue* value = dynamic_cast<InternalBinValue*>(valuePtr.get())) {
            if(visited.find(valuePtr) == visited.end()) {
                getProcCallNPara(value->getLeft(), visited, procs, parameters);
                getProcCallNPara(value->getRight(), visited, procs, parameters);
                visited.insert(valuePtr);
            }
        } else if(InternalUnValue* value = dynamic_cast<InternalUnValue*>(valuePtr.get())) {
            if(visited.find(valuePtr) == visited.end()) {
                getProcCallNPara(value->getRand(), visited, procs, parameters);
                visited.insert(valuePtr);
            }
        } else if(ArrayValue* value = dynamic_cast<ArrayValue*>(valuePtr.get())) {
            for(auto ele : value->getArrayValue())
                if (visited.find(ele) == visited.end()) {
                    getProcCallNPara(ele, visited, procs, parameters);
                    visited.insert(ele);
                }
        } else {
            return;
        }
    }


// 判断左右的rval是否为空的前提
    bool noSameCall(ValuePtr left, ValuePtr right) {
        // 当左右两个值没有相同的函数调用时，其rval的交集就可以为空
        // 1）左右都是局部变量
        // 2）左边或者右边只有一边含形式参数/函数调用
        // 3）都含有函数调用，但函数调用完全不同，包括参数里的函数调用
        std::set<ProcedurePtr> leftProcedure;
        std::set<ValuePtr> leftParameter;
        std::set<ValuePtr> visited;
        getProcCallNPara(left, visited, leftProcedure, leftParameter);

        std::set<ValuePtr> visited1;
        std::set<ProcedurePtr> rightProcedure;
        std::set<ValuePtr> rightParameter;
        getProcCallNPara(right, visited1, rightProcedure, rightParameter);

        if(leftProcedure.size() == 0 && leftParameter.size() == 0 &&
           rightProcedure.size() == 0 && rightParameter.size() == 0)
            return true;

        if(leftProcedure.size() + leftParameter.size() >= 0 && rightParameter.size() + rightProcedure.size() == 0)
            return true;

        if(leftProcedure.size() + leftParameter.size() == 0 && rightParameter.size() + rightProcedure.size() >= 0)
            return true;

//        if(leftProcedure.size() == 0 && rightProcedure.size() == 0 && leftParameter.size() > 0 && rightParameter.size() > 0) {
//            bool noSameParameter = true;
//            for(auto ele : leftParameter) {
//                if(rightParameter.find(ele) != rightParameter.end()) {
//                    noSameParameter = false;
//                    break;
//                }
//            }
//            return noSameParameter;
//        }
        return false;
    }

}
