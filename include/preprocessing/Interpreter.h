//
// Created by Septi on 6/16/2022.
//

#ifndef PLBENCH_INTERPRETER_H
#define PLBENCH_INTERPRETER_H


#include "ASTNode.h"
#include "Value.h"
#include "ValueCommon.h"

extern int yylineno;

class CodeGenBlock{
public:
    ValuePtr returnValue;
    std::map<std::string, ValuePtr> env;
//    std::map<string, bool> isFuncArg;
    std::map<std::string, std::vector<int>> arraySizes;
    std::vector<ValuePtr> parameters;
    std::map<std::string, std::vector<std::string>> arrayNames;
    std::vector<ValuePtr> sequence;
    std::vector<ValuePtr> hdSequence;
    std::vector<ValuePtr> assumptions;
};


typedef std::shared_ptr<CodeGenBlock> CodeGenBlockPtr;


class Interpreter {
private:
    std::vector<CodeGenBlockPtr> blockStack;
    std::vector<ProcValuePtr> procs;
    int numberOfInternal;
    std::set<std::string> nameOfKey;
    std::set<std::string> nameOfPlain;

public:
    static bool isHD;
    static int assumptionLevel;
    static std::vector<std::string> functionName;
    Interpreter() {
        numberOfInternal = 0;
    }

    // revised for debug in 2022.5.23 by Pu Sun
    void blockStackPrint() {
        int idx = 1;
        for (const auto& block : blockStack) {
            std::cout << "------------------ " << idx << " ------------------" << std::endl;
            std::cout << "env : " << std::endl;
            for (const auto& i : block->env) {
                if (i.first != "") {
                    std::cout << i.first << " ";
                }
                std::cout << i.second->getName() << "  -->  toString : " << i.second->toString() << "\n";
            }
            std::cout << std::endl;
            std::cout << "arraysizes : " << std::endl;
            for (auto i : block->arraySizes) {
                std::cout << i.first << " ";
            }
            std::cout << std::endl;
            std::cout << "parameters : " << std::endl;
            for (const auto& i : block->parameters) {
                std::cout << i->getName() << " ";
            }
            std::cout << std::endl;
            std::cout << "arraynames : " << std::endl;
            for (auto i : block->arrayNames) {
                std::cout << i.first << " ";
            }
            std::cout << std::endl;
            std::cout << "sequence : " << std::endl;
            for (auto i : block->sequence) {
                if (i != nullptr)
                    std::cout << i->getName() << "  -->  toString : " << i->toString() << "\n";
            }
            std::cout << std::endl;
            std::cout << "hdsequence : " << std::endl;
            for (auto i : block->hdSequence) {
                std::cout << i->getName() << " ";
            }
            std::cout << "assumptions : " << std::endl;
            for (auto i : block->assumptions) {
                std::cout << i->getName() << " ";
            }
            std::cout << std::endl;
            std::cout << std::endl;
            idx++;
        }
    }

    void addToIntenal(int number) {
        numberOfInternal += number;
    }
//    int getInternal() {
//        return numberOfInternal;
//    }
    void generateCode(ASTNode::NBlock& root) {
        //这里好像不需要在compute前后分别进行pushbBlock和popBlock，
        //因为会多添加一个空的 CodeGenBlockPtr对象和
        // 弹出最后一个CodeGenBlockPtr对象，
        // 如果main函数在最后写，表现就是没有main函数对应的CodeGenBlockPtr实例
        pushBlock();
        CodeGenBlockPtr topDefs = blockStack.back();
        ValuePtr valuePtr = root.compute(*this);
        //popBlock();

        // 因为easyBC程序在程序入口先声明了很多常量数组，所以需要将这些常量数组放在所有函数的env中
        /*for(const auto& block : blockStack){
            for (const auto& tenv : topDefs->env) {
                block->env[tenv.first] = tenv.second;
            }
        }*/
    }

    ValuePtr getValue(std::string type, std::string name, bool isParameter) {
        ValuePtr re = nullptr;
        if(isParameter && type == "int") {
            re = std::make_shared<ConstantValue>(name);
        }
        else if(isParameter) {
            ValuePtr va = std::make_shared<ParameterValue>(name);
            va->setVarType(VarTypeTrans(type));
            re = va;
        /*} else if(type == "private") {
            re = make_shared<PrivateValue>(name);
        } else if(type == "public") {
            re = make_shared<PublicValue>(name);*/
        } else if(type == "internal") {
            re = nullptr;
        } else if(type == "share") {
            re = std::make_shared<ParameterValue>(name);
        /*} else if(type == "random") {
            re = make_shared<RandomValue>(name);*/
        } else if(type == "int") {
            //re = make_shared<RandomValue>(name);
            re = nullptr;
        // 这里我新增几个类型，因为现在的类型位int和uints两类
        } else if(type == "uint") {
            ValuePtr va = std::make_shared<ConstantValue>(name);
            va->setVarType(ASTNode::uint);
            re = va;
        } else if(type == "uint1") {
            ValuePtr va = std::make_shared<ConstantValue>(name);
            va->setVarType(ASTNode::uint1);
            re = va;
        } else if(type == "uint4") {
            ValuePtr va = std::make_shared<ConstantValue>(name);
            va->setVarType(ASTNode::uint4);
            re = va;
        } else if(type == "uint6") {
            ValuePtr va = std::make_shared<ConstantValue>(name);
            va->setVarType(ASTNode::uint6);
            re = va;
        } else if(type == "uint8") {
            ValuePtr va = std::make_shared<ConstantValue>(name);
            va->setVarType(ASTNode::uint8);
            re = va;
        } else if(type == "uint10") {
            ValuePtr va = std::make_shared<ConstantValue>(name);
            va->setVarType(ASTNode::uint10);
            re = va;
        } else if(type == "uint16") {
            ValuePtr va = std::make_shared<ConstantValue>(name);
            va->setVarType(ASTNode::uint16);
            re = va;
        } else if(type == "uint32") {
            ValuePtr va = std::make_shared<ConstantValue>(name);
            va->setVarType(ASTNode::uint32);
            re = va;
        } else if(type == "uint64") {
            ValuePtr va = std::make_shared<ConstantValue>(name);
            va->setVarType(ASTNode::uint64);
            re = va;
        } else if(type == "uint128") {
            ValuePtr va = std::make_shared<ConstantValue>(name);
            va->setVarType(ASTNode::uint128);
            re = va;
        } else {
            assert(false);
        }
        return re;
    }

    void addToEnv(std::string name, ValuePtr value) {
        blockStack.back()->env[name] = value;
    }

    ValuePtr getFromEnv(std::string name) {
//        assert(blockStack.back()->env.count(name));
//        if(blockStack.back()->env.count(name) == 0)
//            return nullptr;
//        return blockStack.back()->env[name];

        for(auto it = blockStack.rbegin(); it != blockStack.rend(); it++){
            if( (*it)->env.find(name) != (*it)->env.end() ){
                return (*it)->env[name];
            }
        }
        return nullptr;
    }

    void pushBlock() {
        CodeGenBlockPtr codeGenBlock = std::make_shared<CodeGenBlock>();
        codeGenBlock->returnValue = nullptr;
        blockStack.push_back(codeGenBlock);
    }

    void popBlock() {
        CodeGenBlockPtr codeGenBlock = blockStack.back();
        blockStack.pop_back();
    }

    void addParameter(std::vector<ValuePtr>& para) {
        blockStack.back()->parameters.insert(blockStack.back()->parameters.end(), para.begin(), para.end());
    }

    void addParameter(const ValuePtr& para) {
        blockStack.back()->parameters.push_back(para);
    }

    const std::vector<ValuePtr>& getParameter() {
        return blockStack.back()->parameters;
    }

    void addNameOfKey(std::string res) {
        nameOfKey.insert(res);
    }

    void addNameOfPlain(std::string res) {
        nameOfPlain.insert(res);
    }

    std::set<std::string> getNameOfPlain() {
        return nameOfPlain;
    }

    std::set<std::string> getNameOfKey() {
        return nameOfKey;
    }

    std::vector<int> getArraySize(std::string name){
        for(auto it=blockStack.rbegin(); it!=blockStack.rend(); it++){
            if( (*it)->arraySizes.find(name) != (*it)->arraySizes.end() ){
                return (*it)->arraySizes[name];
            }
        }
        return blockStack.back()->arraySizes[name];
    }

    void setArraySize(std::string name, std::vector<int> value){
//        cout << "setArraySize: " << name << ": " << value.size() << endl;
        blockStack.back()->arraySizes[name] = value;
//        cout << "blockStack.back()->arraySizes.size()" << blockStack.back()->arraySizes.size() << endl;
    }

    void setArrayNames(std::string name, std::vector<std::string>& names) {
        blockStack.back()->arrayNames[name] = names;
    }

    std::vector<std::string> getArrayNames(std::string name) {
        if(blockStack.back()->arrayNames.count(name) == 0)
            assert(false);
        return blockStack.back()->arrayNames[name];
    }

    void setCurrentReturnValue(ValuePtr value){
        blockStack.back()->returnValue = value;
    }

    ValuePtr getCurrentReturnValue(){
        return blockStack.back()->returnValue;
    }

    // 新增该函数为了可以获取所有的blockStack，用于roundfunction和cipherfunction中添加全局的evn
    std::vector<CodeGenBlockPtr> getblockStack() {
        return blockStack;
    }


    std::vector<ValuePtr>& getSequence() {
        return blockStack.back()->sequence;
    }

    void addToSequence(ValuePtr loc) {
        blockStack.back()->sequence.push_back(loc);
    }

    void addToHDSequence(ValuePtr loc) {
        blockStack.back()->hdSequence.push_back(loc);
    }

    std::vector<ValuePtr> getHDSequence() {
        return blockStack.back()->hdSequence;
    }

    std::map<std::string, ValuePtr> getEnv() {
        return blockStack.back()->env;
    }

    void addProc(ValuePtr proc) {
        ProcValuePtr procPtr = std::dynamic_pointer_cast<ProcValue>(proc);
        procs.push_back(procPtr);
    }

    const std::vector<ProcValuePtr> &getProcs() const {
        return procs;
    }

    ValuePtr getProc(std::string name) {
        for(auto ele : procs) {
            if(ProcValue* proc = dynamic_cast<ProcValue*>(ele.get())) {
                if(proc->getProcName() == name) {
                    return ele;
                } else {
                    continue;
                }
            } else {
                assert(false);
                return nullptr;
            }
        }
        assert(false);
        return nullptr;
    }

    int value_of(ValuePtr value) {
        if(ConcreteNumValue* number = dynamic_cast<ConcreteNumValue*>(value.get())) {
            return number->getNumer();
        } else {
            assert(false);
            return 0;
        }
    }


    void addAssumptions(ValuePtr ass) {
        blockStack.back()->assumptions.push_back(ass);
    }

    std::vector<ValuePtr> getAssumptions() {
        return blockStack.back()->assumptions;
    }

    // 输入数组名称name，以及数组的index的list，算出所有的数组下标
    // 当expList的长度正好跟Array的实际长度一致，那么返回的就是一个值
    // 当expList的长度小于Array的实际长度的时候，返回的是一个数组的值
    // 大于的时候，报错
    // 如果index中含有变量，那么只能返回值。所以，
    // 如果index全部都是数字，或者是环境中有值的变量，就可以算得具体的下标的值
    // 如果index中含有变量，不知道具体值的时候，此时index应该是什么呢？
    std::vector<int> computeIndex(std::string name, ASTNode::NExpressionListPtr expList) {
        auto sizeVec = getArraySize(name);

        int index = 0;
        int dimons = sizeVec.size();
        for(int i = 0; i < expList->size(); i++) {
            auto arraySize = expList->at(i);
            if(arraySize->getTypeName() == "NInteger") {
                ASTNode::NIntegerPtr ident = std::dynamic_pointer_cast<ASTNode::NInteger>(arraySize);
                if(i != dimons - 1) {
                    int temp = 1;
                    for(int j = i + 1; j < sizeVec.size(); j++)
                        temp *= sizeVec[j];
                    index += temp * ident->getValue();
                }
                else
                    index += ident->getValue();
            } else if(arraySize->getTypeName() == "NIdentifier") {
                ASTNode::NIdentifierPtr ident = std::dynamic_pointer_cast<ASTNode::NIdentifier>(arraySize);
                if(i != dimons - 1) {
                    int temp = 1;
                    for (int j = i + 1; j < sizeVec.size(); j++)
                        temp *= sizeVec[j];
                    index += temp * value_of(ident->compute(*this));
                }
                else
                    index += value_of(ident->compute(*this));
            } else if(arraySize->getTypeName() == "NBinaryOperator") {
                ValuePtr arraySizeValue = arraySize->compute(*this);
                if(!ValueCommon::isNoParameter(arraySizeValue)) {
                    // 当arraySizeValue中含有parameter的时候
                }
                int value = value_of(arraySizeValue);
                if(i != dimons - 1) {
                    int temp = 1;
                    for (int j = i + 1; j < sizeVec.size(); j++)
                        temp *= sizeVec[j];
                    index += temp * value;
                }
                else
                    index += value;
            }
                // 这里我添加了一个新的分支，用于处理数组的index是另一个数组index的情况，如：rtn[pbox[i]]
                /*else if (arraySize->getTypeName() == "NArrayIndex") {
                    expList->at(i)->compute(*this);
                }*/
            else {
                std::cout << "debug info : " << arraySize->getTypeName() << std::endl;
                assert(false);
            }
        }

        int number = 1;
        if(expList->size() < sizeVec.size()) {
            // 意味着要返回一个数组的值了，不是单个的值
            // from index to
            // 计算个数就是后面所有的乘起来
            for(int i = expList->size(); i < sizeVec.size(); i++) {
                number *= sizeVec.at(i);
            }
        }

        std::vector<int> res;
        if(expList->size() == sizeVec.size()) {
            res.push_back(index);
        } else {
            for(int i = index; i < index + number; i++) {
                res.push_back(i);
            }
        }
        return res;
    }


    /*
     * 在这里我们要计算index，在这里我们分为两种情况：
     * 1. index是包含输入参数的，此时index具体的值不能被计算出来，只能用symbol方式表示出来；
     * 2. index没有包含输入参数，index具体值可以被计算出，此时计算index的具体值又分为三种情况：
     *      1）当数组是一维的时候，index的值直接计算即可；
     *      2）当数组是二维的时候，index的值需要由两个值来决定，具体的，
     *         数组a中，a[i1][i2]的具体index值需要求解的是 i1 * rowSize + i2
     *      3）当数组是一维，取到数组某一个元素时，又对该元素的某位bit取值，如 rc[r-1][i]
     * */
    std::vector<ValuePtr> computeIndexWithSymbol(std::string name, ASTNode::NExpressionListPtr expList) {

        auto sizeVec = getArraySize(name);
        ValuePtr index = std::make_shared<ConcreteNumValue>("index", 0);
        int dimons = sizeVec.size();
        for(int i = 0; i < expList->size(); i++) {
            auto arraySize = expList->at(i);
            if(arraySize->getTypeName() == "NInteger") {
                ASTNode::NIntegerPtr ident = std::dynamic_pointer_cast<ASTNode::NInteger>(arraySize);
                ValuePtr identValue = std::make_shared<ConcreteNumValue>("ident", ident->getValue());
                if(i != dimons - 1) {
//                    int temp = 1;
                    ValuePtr temp = std::make_shared<ConcreteNumValue>("1", 1);
                    for(int j = i + 1; j < sizeVec.size(); j++) {
                        std::shared_ptr<ConcreteNumValue> sizeVecJ = std::make_shared<ConcreteNumValue>("j", sizeVec[j]);
                        temp = std::make_shared<InternalBinValue>("temp", temp, sizeVecJ, ASTNode::Operator::FFTIMES);
                    }
                    temp = std::make_shared<InternalBinValue>("temp", temp, identValue, ASTNode::Operator::FFTIMES);
                    index = std::make_shared<InternalBinValue>("index", index, temp, ASTNode::Operator::ADD);
                }
                else {
                    // 这里我们先注释，因为实际上是可以直接得到具体的index的值的
                    //index = std::make_shared<InternalBinValue>("index", index, identValue, ASTNode::Operator::ADD);
                    index = identValue;
                }
            }
            else if(arraySize->getTypeName() == "NIdentifier") {
                ASTNode::NIdentifierPtr ident = std::dynamic_pointer_cast<ASTNode::NIdentifier>(arraySize);
                ValuePtr identValue = ident->compute(*this);
                //这里注释是因为有一些输入相关的表达式时，其值可能无法计算，
                // 此时我们需要想办法对其进行赋值，或者用其他值替代 - by Sun Pu
                if(!ValueCommon::isNoParameter(identValue)) {
                    // 这里直接将表达式保存了
                    index = identValue;
                    //assert(false);
                }
                else if(i != dimons - 1) {
//                    int temp = 1;
                    ValuePtr temp = std::make_shared<ConcreteNumValue>("1", 1);
                    for(int j = i + 1; j < sizeVec.size(); j++) {
                        std::shared_ptr<ConcreteNumValue> sizeVecJ = std::make_shared<ConcreteNumValue>("j", sizeVec[j]);
                        temp = std::make_shared<InternalBinValue>("temp", temp, sizeVecJ, ASTNode::Operator::FFTIMES);
                    }
                    temp = std::make_shared<InternalBinValue>("temp", temp, identValue, ASTNode::Operator::FFTIMES);
                    index = std::make_shared<InternalBinValue>("index", index, temp, ASTNode::Operator::ADD);
                }
                else {
                    //index = std::make_shared<InternalBinValue>("index", index, identValue, ASTNode::Operator::ADD);
                    index = identValue;
                }
            }
            else if(arraySize->getTypeName() == "NBinaryOperator" || arraySize->getTypeName() == "NArrayIndex") {
                ValuePtr arraySizeValue = arraySize->compute(*this);
                // 如果arraysize中是含有参数的表达式
                if(!ValueCommon::isNoParameter(arraySizeValue)) {
                    // 当arraySizeValue中含有parameter的时候
                    index = arraySizeValue;
                    // 也将该index标记为symbol, 这里我先不改index的symbol的标记了。后续有用到再进行更改，现在debug的有点混乱
                    // index->set_symbol_value_f();
                    //assert(false);
                }
                // 如果dimons为1，那么数组为一维数组，所以在这里我们添加一部分约束，即也要求dimons为一维数组。
                else if(i != dimons - 1 and dimons != 1) {
                    ValuePtr temp = std::make_shared<ConcreteNumValue>("1", 1);
                    for(int j = i + 1; j < sizeVec.size(); j++) {
                        std::shared_ptr<ConcreteNumValue> sizeVecJ = std::make_shared<ConcreteNumValue>("j", sizeVec[j]);
                        temp = std::make_shared<InternalBinValue>("temp", temp, sizeVecJ, ASTNode::Operator::FFTIMES);
                    }
                    temp = std::make_shared<InternalBinValue>("temp", temp, arraySizeValue, ASTNode::Operator::FFTIMES);
                    index = std::make_shared<InternalBinValue>("index", index, temp, ASTNode::Operator::ADD);
                }
                // 当数组为一维数组，且index要访问数组某元素的某个bit时
                // 此时需要将两次不同的数组访问用操作符联系起来，因此新定义操作符 "NEWINDEX"
                // 第二个index访问某个元素的某个bit的valueptr即为 arraySizeValue
                else if (i != dimons - 1 and dimons == 1 ) {
                    index = std::make_shared<InternalBinValue>("index", index, arraySizeValue, ASTNode::Operator::NEWINDEX);
                }
                else {
                    //index = std::make_shared<InternalBinValue>("index", index, arraySizeValue, ASTNode::Operator::ADD);
                    index = arraySizeValue;
                }
//                }
            } else {
                assert(false);
            }
        }

        int number = 1;
        if(expList->size() < sizeVec.size()) {
            // 意味着要返回一个数组的值了，不是单个的值
            // from index to
            // 计算个数就是后面所有的乘起来
            for(int i = expList->size(); i < sizeVec.size(); i++) {
                number *= sizeVec.at(i);
            }
        }

        std::vector<ValuePtr> res;
        if(expList->size() == sizeVec.size()) {
            res.push_back(index);
        } else {
            res.push_back(index);
            for(int i = 1; i < number; i++) {
                ValuePtr temp = std::make_shared<ConcreteNumValue>("i", i);
                res.push_back(std::make_shared<InternalBinValue>(std::to_string(i), index, temp, ASTNode::Operator::ADD));
            }
        }
        return res;
    }


    static ASTNode::Type VarTypeTrans(std::basic_string<char> type) {
        if (type == "uint")
            return ASTNode::Type::uint;
        else if (type == "uint1")
            return ASTNode::Type::uint1;
        else if (type == "uint4")
            return ASTNode::Type::uint4;
        else if (type == "uint6")
            return ASTNode::Type::uint6;
        else if (type == "uint8")
            return ASTNode::Type::uint8;
        else if (type == "uint10")
            return ASTNode::Type::uint10;
        else if (type == "uint16")
            return ASTNode::Type::uint16;
        else if (type == "uint32")
            return ASTNode::Type::uint32;
        else if (type == "uint64")
            return ASTNode::Type::uint64;
        else if (type == "uint128")
            return ASTNode::Type::uint128;
        else if (type == "uint256")
            return ASTNode::Type::uint256;
        else if (type == "uint512")
            return ASTNode::Type::uint512;
        return ASTNode::Type::uint;
    }

    static ASTNode::Type int2Type(int size) {
        switch (size) {
            case 0:
                return ASTNode::Type::uint;
            case 1:
                return ASTNode::Type::uint1;
            case 4:
                return ASTNode::Type::uint4;
            case 5:
                return ASTNode::Type::uint6;
            case 6:
                return ASTNode::Type::uint6;
            case 7:
                return ASTNode::Type::uint8;
            case 8:
                return ASTNode::Type::uint8;
            case 9:
                return ASTNode::Type::uint10;
            case 10:
                return ASTNode::Type::uint10;
            case 11:
                return ASTNode::Type::uint16;
            case 12:
                return ASTNode::Type::uint16;
            case 13:
                return ASTNode::Type::uint16;
            case 14:
                return ASTNode::Type::uint16;
            case 15:
                return ASTNode::Type::uint16;
            case 16:
                return ASTNode::Type::uint16;
            case 32:
                return ASTNode::Type::uint32;
            case 64:
                return ASTNode::Type::uint64;
            case 128:
                return ASTNode::Type::uint128;
            case 512:
                return ASTNode::Type::uint512;
            default:
                return ASTNode::Type::uint;
        }
    }


    static int getVarTypeSize(ASTNode::Type type) {
        if (type == ASTNode::Type::uint) {
            return 0;
        }
        else if (type == ASTNode::Type::uint1) {
            return 1;
        }
        else if (type == ASTNode::Type::uint4) {
            return 4;
        }
        else if (type == ASTNode::Type::uint6) {
            return 6;
        }
        else if (type == ASTNode::Type::uint8) {
            return 8;
        }
        else if (type == ASTNode::Type::uint10) {
            return 10;
        }
        else if (type == ASTNode::Type::uint16) {
            return 16;
        }
        else if (type == ASTNode::Type::uint32) {
            return 32;
        }
        else if (type == ASTNode::Type::uint64) {
            return 64;
        }
        else if (type == ASTNode::Type::uint128) {
            return 128;
        }
        else if (type == ASTNode::Type::uint256) {
            return 256;
        }
        else if (type == ASTNode::Type::uint512) {
            return 512;
        }
        return 0;
    }


    /*// type system
    bool typingBin(InternalBinValue iBV) {
        if (iBV.getLeft()->getValueType() == )
    }*/
};

#endif //PLBENCH_INTERPRETER_H
