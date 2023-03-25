//
// Created by Septi on 6/27/2022.
//

#ifndef PLBENCH_THREEADDRESSNODE_H
#define PLBENCH_THREEADDRESSNODE_H

#include <string>
#include "ASTNode.h"
#include <set>
using namespace std;
enum NodeType {
    CONSTANT,
    UNKNOWN,
    INT,
    PARAMETER,
    RETURN,
    FUNCTION,
    INTERNALINLINE,
    // new added by sunpu
    ARRAY,
    BOX,
    INDEX, // 用于touint转三地址时返回的vector三地址对象的索引标识
    UINT,
    UINT1,
    UINT4,
    UINT6,
    UINT8,
    UINT10,
    UINT16,
    UINT32,
    UINT64,
    UINT128,
    UINT256,
    UINT512,
};

enum Distribution {
    RUD,
    SID,
    SDD,
    UKD
};

const string TokenPrivate = "PRIVATE";
const string TokenPreshare = "PRESHARE";
const string TokenRandom = "RANDOM";
const string TokenPublic = "PUBLIC";
const string TokenInternal = "INTERNAL";
const string TokenConstant = "CONSTANT";
const string TokenUnknown = "UNKNOWN";
const string TokenInt = "INT";
const string TokenRUD = "RUD";
const string TokenUKD = "UKD";
const string TokenSID = "SID";
const string TokenSDD = "SDD";


class ThreeAddressNode {

private:
    ASTNode::Operator op;
    ThreeAddressNodePtr lhs;
    ThreeAddressNodePtr rhs;
    vector<ThreeAddressNodeWeakPtr> parents;
    int numberOfChild;

    string nodeName;
    int SSAIndex;
    int lineNumber;
    NodeType nodeType;
    int indexOfCall;

    Distribution distribution;

    set<ThreeAddressNodePtr> SupportV;
    set<ThreeAddressNodePtr> UniqueM;
    set<ThreeAddressNodePtr> PerfectM;
    set<ThreeAddressNodePtr> Dominant;


public:
    ThreeAddressNode(string nodeName, const ThreeAddressNodePtr &lhs, const ThreeAddressNodePtr &rhs,
                     ASTNode::Operator op, NodeType nodeType) :
                     op(op),
                     lhs(lhs),
                     rhs(rhs),
                     nodeName(nodeName),
                     nodeType(nodeType) {
        SSAIndex = 0;
        lineNumber = 0;
        distribution = Distribution::UKD;
        numberOfChild = 0;

//        if(lhs == nullptr && rhs == nullptr)
//            numberOfChild = 1;
//        else if(lhs != nullptr && rhs == nullptr)
//            numberOfChild = lhs->numberOfChild;
//        else if(lhs != nullptr && rhs != nullptr)
//            numberOfChild = lhs->numberOfChild + rhs->numberOfChild;
//        else
//            assert(false);
    };

    ThreeAddressNode(string nodeName, ThreeAddressNodePtr node) :
                     nodeName(nodeName),
                     lhs(node->lhs),
                     rhs(node->rhs),
                     op(node->op),
                     nodeType(node->nodeType),
                     distribution(node->getDistribution())  {
        lineNumber = 0;
        SSAIndex = 0;
        numberOfChild = 0;
//        numberOfChild = node->numberOfChild;
    };

    ThreeAddressNode(ThreeAddressNodePtr& addressPtr, map<string, ThreeAddressNodePtr>& nodeMap) {
        nodeName = addressPtr->nodeName;
        if(addressPtr->lhs != nullptr) {
            if(addressPtr->lhs->getNodeType() == NodeType::CONSTANT && nodeMap.count(addressPtr->lhs->getIDWithOutLineNumber()) == 0) {
                nodeMap[addressPtr->lhs->getIDWithOutLineNumber()] = make_shared<ThreeAddressNode>(addressPtr->lhs->getNodeName(), nullptr, nullptr, ASTNode::Operator::NULLOP, NodeType::CONSTANT);
            }
            lhs = nodeMap[addressPtr->lhs->getIDWithOutLineNumber()];
            if(lhs == nullptr) {
                nodeMap[addressPtr->lhs->getIDWithOutLineNumber()] = make_shared<ThreeAddressNode>(addressPtr->lhs, nodeMap);
            }
            lhs = nodeMap[addressPtr->lhs->getIDWithOutLineNumber()];
        }
        if(addressPtr->rhs != nullptr) {
            rhs = nodeMap[addressPtr->rhs->getIDWithOutLineNumber()];
            if(rhs == nullptr) {
                nodeMap[addressPtr->rhs->getIDWithOutLineNumber()] = make_shared<ThreeAddressNode>(addressPtr->rhs, nodeMap);
            }
            rhs = nodeMap[addressPtr->rhs->getIDWithOutLineNumber()];
        }
        op = addressPtr->op;
        nodeType = addressPtr->nodeType;
        SSAIndex = addressPtr->SSAIndex;
        lineNumber = addressPtr->lineNumber;
        distribution = addressPtr->distribution;
        numberOfChild = 0;
//        numberOfChild = addressPtr->numberOfChild;
    }

    ThreeAddressNode(ThreeAddressNodePtr& addressPtr, map<ThreeAddressNodePtr, ThreeAddressNodePtr>& nodeMap) {
        nodeName = addressPtr->nodeName;
        if(addressPtr->lhs != nullptr) {
            assert(nodeMap.count(addressPtr->lhs) != 0);
            lhs = nodeMap[addressPtr->lhs];
//            if(lhs == nullptr) {
//                nodeMap[addressPtr->lhs] = make_shared<ThreeAddressNode>(addressPtr->lhs, nodeMap);
//            }
//            lhs = nodeMap[addressPtr->lhs->getIDWithOutLineNumber()];
        }
        if(addressPtr->rhs != nullptr) {
            assert(nodeMap.count(addressPtr->rhs) != 0);
            rhs = nodeMap[addressPtr->rhs];
//            if(rhs == nullptr) {
//                nodeMap[addressPtr->rhs->getIDWithOutLineNumber()] = make_shared<ThreeAddressNode>(addressPtr->rhs, nodeMap);
//            }
//            rhs = nodeMap[addressPtr->rhs->getIDWithOutLineNumber()];
        }
        op = addressPtr->op;
        nodeType = addressPtr->nodeType;
        SSAIndex = addressPtr->SSAIndex;
        lineNumber = addressPtr->lineNumber;
        distribution = addressPtr->distribution;
        numberOfChild = 0;
//        numberOfChild = addressPtr->numberOfChild;
    }

    ThreeAddressNode(ThreeAddressNodePtr& addressPtr) {
        nodeName = addressPtr->nodeName;
        op = addressPtr->op;
        nodeType = addressPtr->nodeType;
        SSAIndex = addressPtr->SSAIndex;
        lineNumber = addressPtr->lineNumber;
//        numberOfChild = addressPtr->numberOfChild;
        lhs = nullptr;
        rhs = nullptr;
        indexOfCall = addressPtr->indexOfCall;
        numberOfChild = 0;

    }

    ThreeAddressNodePtr createNodePtr(string nodeName, const ThreeAddressNodePtr &lhs, const ThreeAddressNodePtr &rhs,
                                      ASTNode::Operator op, NodeType nodeType) {
        return make_shared<ThreeAddressNode>(nodeName, lhs, rhs, op, nodeType);
    }

    const string &getNodeName() const {
        return nodeName;
    }

    void setNodeName(const string &nodeName) {
        ThreeAddressNode::nodeName = nodeName;
    }

    NodeType getNodeType() const {
        return nodeType;
    }

    void setNodeType(NodeType nodeType) {
        ThreeAddressNode::nodeType = nodeType;
    }

    const ThreeAddressNodePtr &getRhs() const {
        return rhs;
    }

    const ThreeAddressNodePtr &getLhs() const {
        return lhs;
    }

    void setLhs(ThreeAddressNodePtr newPtr) {
        this->lhs = newPtr;

//        int newNumber = (this->rhs == nullptr)? newPtr->numberOfChild : newPtr->numberOfChild + rhs->numberOfChild;
//        if(newNumber != this->numberOfChild)
//            this->numberOfChild = newNumber;
    }

    void setRhs(ThreeAddressNodePtr newPtr) {
        this->rhs = newPtr;
//        int newNumber = newPtr->numberOfChild + lhs->numberOfChild;
//        if(newNumber != this->numberOfChild)
//            this->numberOfChild = newNumber;
    }

    ASTNode::Operator getOp() const {
        return op;
    }

    void setOp(ASTNode::Operator newOp) {
        this->op = newOp;
    }

    static ThreeAddressNodePtr copyNode(ThreeAddressNodePtr node, map<string, ThreeAddressNodePtr>& saved) {
        if(!node)
            return nullptr;
        if(saved.count(node->getNodeName()) != 0)
            return saved[node->getNodeName()];

        ThreeAddressNodePtr newNode = nullptr;//make_shared<ThreeAddressNode>(node, saved);
        if(node->getNodeType() == NodeType::CONSTANT){
            newNode = make_shared<ThreeAddressNode>(node);
        }
        /*else if(node->getNodeType() == NodeType::RANDOM) {
            newNode = make_shared<ThreeAddressNode>(node);
        }*/
        else if(node->getNodeType() == NodeType::PARAMETER) {
            newNode = make_shared<ThreeAddressNode>(node);
        }
        /*else if(node->getNodeType() == NodeType::INTERNAL) {
            ThreeAddressNodePtr lhs = copyNode(node->getLhs(), saved);

            ThreeAddressNodePtr rhs = nullptr;
            if(node->getRhs()) {
                rhs = copyNode(node->getRhs(), saved);
            }

            newNode = make_shared<ThreeAddressNode>(node);
            newNode->setLhs(lhs);
            newNode->setRhs(rhs);

            lhs->addParents(newNode);
            if(rhs) {
                rhs->addParents(newNode);
            }

        }*/
        else if(node->getNodeType() == NodeType::INTERNALINLINE) {
            ThreeAddressNodePtr lhs = copyNode(node->getLhs(), saved);
            newNode = make_shared<ThreeAddressNode>(node);
            newNode->setLhs(lhs);
        }
        else if(node->getNodeType() == NodeType::FUNCTION) {
            newNode = make_shared<ThreeAddressNode>(node);
        }
        else {
            assert(false);
        }
        saved[node->getNodeName()] = newNode;

        return newNode;
    }

    static ThreeAddressNodePtr copyNodeWithPath(string sufix, ThreeAddressNodePtr node, map<string, ThreeAddressNodePtr>& saved, map<string, ThreeAddressNodePtr>& formalToActual) {
        if(!node)
            return nullptr;
        if(saved.count(node->getNodeName()) != 0 && node->getNodeType() != NodeType::PARAMETER)
            return saved[node->getNodeName()];

        ThreeAddressNodePtr newNode = nullptr;//make_shared<ThreeAddressNode>(node, saved);
        if(node->getOp() == ASTNode::Operator::CALL) {
            cout << "hello" << endl;
        }
        if(node->getNodeName() == "a0")
            cout << "hello" << endl;
        /*if(node->getNodeType() == NodeType::RANDOM) {
            newNode = make_shared<ThreeAddressNode>(node->getNodeName() + sufix , node);
        }*/
        else if(node->getNodeType() == NodeType::PARAMETER) {
            assert(formalToActual.count(node->getNodeName()) != 0);
            newNode = formalToActual[node->getNodeName()];
        }
        /*else if(node->getNodeType() == NodeType::INTERNAL) {
            ThreeAddressNodePtr lhs = copyNodeWithPath(sufix, node->getLhs(), saved, formalToActual);

            ThreeAddressNodePtr rhs = nullptr;
            if(node->getRhs()) {
                rhs = copyNodeWithPath(sufix ,node->getRhs(), saved, formalToActual);
            }

            newNode = make_shared<ThreeAddressNode>(node->getNodeName() + sufix, node);
            newNode->setLhs(lhs);
            newNode->setRhs(rhs);

            lhs->addParents(newNode);
            if(rhs) {
                rhs->addParents(newNode);
            }

        }*/
        else if(node->getNodeType() == NodeType::INTERNALINLINE) {
            ThreeAddressNodePtr lhs = copyNodeWithPath(sufix, node->getLhs(), saved, formalToActual);
            newNode = make_shared<ThreeAddressNode>(node->getNodeName() + sufix, node);
            newNode->setLhs(lhs);
        }
        else if(node->getNodeType() == NodeType::FUNCTION) {
            newNode = make_shared<ThreeAddressNode>(node->getNodeName() + sufix, node);
        }
        else {
            assert(false);
        }
        saved[node->getNodeName()] = newNode;

        return newNode;
    }



    string printNodeType() {
        switch(this->getNodeType()) {
            /*case NodeType::RANDOM:
                return TokenRandom;*/
            case NodeType ::CONSTANT:
                return TokenConstant;
            /*case NodeType ::UNKNOWN:
                return TokenUnknown;
            case NodeType ::INTERNAL:
                return TokenInternal;
            case NodeType ::PRIVATE:
                return TokenPrivate;
            case NodeType ::PUBLIC:
                return TokenPublic;
            case NodeType ::PRESHARE:
                return TokenPreshare;*/
            case NodeType ::INT:
                return TokenInt;
            default:
                return TokenUnknown;
        }
    }

    string printNodeDistribution() {
        switch(this->getDistribution()) {
            case Distribution ::RUD:
                return TokenRUD;
            case Distribution ::SID:
                return TokenSID;
            case Distribution ::SDD:
                return TokenSDD;
            case Distribution ::UKD:
                return TokenUKD;
        }
    }

    static void getRandom(ThreeAddressNodePtr node, set<string>& randoms) {
        if(node == nullptr)
            return;
        /*if(node->getNodeType() == NodeType::RANDOM)
            randoms.insert(node->getNodeName());*/
        getRandom(node->getLhs(),randoms);
        getRandom(node->getRhs(),randoms);
    }

    static void getKeys(ThreeAddressNodePtr node, set<string>& keys) {
        if(node == nullptr)
            return;
        /*if(node->getNodeType() == NodeType::PRIVATE)
            keys.insert(node->getNodeName());*/
        getKeys(node->getLhs(),keys);
        getKeys(node->getRhs(),keys);
    }



    static void getPublics(ThreeAddressNodePtr node, set<string>& publics) {
        if(node == nullptr)
            return;
        /*if(node->getNodeType() == NodeType::PUBLIC)
            publics.insert(node->getNodeName());*/
        getPublics(node->getLhs(), publics);
        getPublics(node->getRhs(), publics);
    }
    void prettyPrint() {
        if(nodeType == NodeType::INT)
            return;
        /*if(nodeType == NodeType::RANDOM) {
            cout << getID() + " = $" << "(" << printNodeType() << ")"<< printNodeDistribution() << endl;
            return;
        }*/

        if(lhs == nullptr && rhs == nullptr) {
            cout << getID() << "(" << printNodeType() << ")" << printNodeDistribution() << endl;
            return;
        }
        string result = getID() + " = " + lhs->getID();
        string oper;
        if(rhs != nullptr) {
            switch (op) {
                case ASTNode::Operator::ADD:
                    oper = "+";
                    break;
                case ASTNode::Operator::MINUS:
                    oper = "-";
                    break;
                case ASTNode::Operator::FFTIMES:
                    oper = "*";
                    break;
                case ASTNode::Operator::AND:
                    oper = "&";
                    break;
                case ASTNode::Operator::XOR:
                    oper = "^";
                    break;
                case ASTNode::Operator::OR:
                    oper = "|";
                    break;
                /*case ASTNode::Operator::LSL:
                    oper = "<<";
                    break;*/
                /*case ASTNode::Operator::NE:
                    oper = "!=";
                    break;*/
                case ASTNode::Operator ::RSH:
                    oper = ">>";
                    break;
                case ASTNode::Operator ::LSH:
                    oper = "<<";
                    break;
                /*case ASTNode::Operator ::POW2:
                    oper = "POW2";
                    break;
                case ASTNode::Operator ::POW4:
                    oper = "POW4";
                    break;
                case ASTNode::Operator ::POW16:
                    oper = "POW16";
                    break;
                case ASTNode::Operator ::AFFINE:
                    oper = "AFFINE";
                    break;*/
                default:
                    assert(false);
            }

            result += " " + oper + " " + rhs->getID() + "(" + printNodeType() + ")";
        } else {
            result += " " + oper;
        }

        result += printNodeDistribution();
        cout << result << endl;
    }

    string prettyPrint4() {
        /*if(nodeType == NodeType::RANDOM) {
            return "$" + getNodeName();
        }*/
        string result = getNodeName() + " = " + lhs->getNodeName();
        string oper;
        if(rhs != nullptr) {
            switch (op) {
                case ASTNode::Operator::ADD:
                    oper = "+";
                    break;
                case ASTNode::Operator::MINUS:
                    oper = "-";
                    break;
                case ASTNode::Operator::FFTIMES:
                    oper = "*";
                    break;
                case ASTNode::Operator::AND:
                    oper = "&";
                    break;
                case ASTNode::Operator::XOR:
                    oper = "^";
                    break;
                case ASTNode::Operator::OR:
                    oper = "|";
                    break;
                /*case ASTNode::Operator::LSL:
                    oper = "<<";
                    break;
                case ASTNode::Operator::NE:
                    oper = "!=";
                    break;*/
                case ASTNode::Operator ::RSH:
                    oper = ">>";
                    break;
                case ASTNode::Operator ::LSH:
                    oper = "<<";
                    break;
                /*case ASTNode::Operator ::POW2:
                    oper = "POW2";
                    break;
                case ASTNode::Operator ::POW4:
                    oper = "POW4";
                    break;
                case ASTNode::Operator ::POW16:
                    oper = "POW16";
                    break;
                case ASTNode::Operator ::AFFINE:
                    oper = "AFFINE";
                    break;*/
                default:
                    assert(false);
            }

            result += " " + oper + " " + rhs->getNodeName();
        } else {
            result += " " + oper;
        }

        return result + "\n";
    }

    string prettyPrint5() {

        if(nodeType == NodeType::FUNCTION) {
            return "CALL " + getNodeName() ;
        }
        /*if(nodeType == NodeType::RANDOM) {
            return "$" + getNodeName();
        }*/
        if(!lhs)
            return getNodeName();
        string left =  lhs->prettyPrint5();
        string right = "";
        string result = "";
        string oper;

        switch (op) {
            case ASTNode::Operator::ADD:
                oper = "+";
                break;
            case ASTNode::Operator::MINUS:
                oper = "-";
                break;
            case ASTNode::Operator::FFTIMES:
                oper = "*";
                break;
            case ASTNode::Operator::AND:
                oper = "&";
                break;
            case ASTNode::Operator::XOR:
                oper = "^";
                break;
            case ASTNode::Operator::OR:
                oper = "|";
                break;
            /*case ASTNode::Operator::LSL:
                oper = "<<";
                break;
            case ASTNode::Operator::NE:
                oper = "!=";
                break;*/
            case ASTNode::Operator ::RSH:
                oper = ">>";
                break;
            case ASTNode::Operator ::LSH:
                oper = "<<";
                break;
            /*case ASTNode::Operator ::POW2:
                oper = "POW2";
                break;
            case ASTNode::Operator ::POW4:
                oper = "POW4";
                break;
            case ASTNode::Operator ::POW16:
                oper = "POW16";
                break;
            case ASTNode::Operator ::AFFINE:
                oper = "AFFINE";
                break;
            case ASTNode::Operator ::XTIMES:
                oper = "XTIMES";
                break;
            case ASTNode::Operator ::TRCON:
                oper = "TRCON";
                break;*/
            case ASTNode::Operator::NULLOP:
                oper = "";
                break;
            case ASTNode::Operator::CALL:
                oper = "[" + to_string(indexOfCall) + "](" + nodeName + ")";
                break;
            /*case ASTNode::Operator::TABLELUT:
                oper = "TABLUT";
                break;
            case ASTNode::Operator::POL:
                oper = "POL";
                break;
            case ASTNode::Operator::LIN:
                oper = "LIN";
                break;*/
            case ASTNode::Operator::NOT:
                oper = "~";
                break;
            /*case ASTNode::Operator::TABLELUT4:
                oper = "TABLUT4";
                break;*/
            // 新增两个操作符，TOUINT和BOXINDEX
            case ASTNode::Operator::TOUINT:
                oper = "touint";
                break;
            case ASTNode::Operator::BOXINDEX:
                oper = "boxindex";
                break;
            case ASTNode::Operator::BOXOP:
                oper = "boxop";
                break;
            case ASTNode::Operator::SYMBOLINDEX:
                oper = "symbolindex";
                break;
            default:
                assert(false);
        }

        if(rhs != nullptr) {
            right = rhs->prettyPrint5();
            result = "(" + left + " " + oper + " " + right + ")";
        } else {
            result = "(" + oper + " " + left + ")";
        }

        return result;
    }

    void setNumberOfChild(int n) {
        this->numberOfChild = n;
    }

    int getNumberOfChild() {
        return this->numberOfChild;
    }

    string prettyPrint3() {
//        if(nodeType == NodeType::INT)
//            return;
        /*if(nodeType == NodeType::RANDOM) {
            return getID() + " = $" + "(" + printNodeType() + ")" + printNodeDistribution() + "\n";
        }*/

        if(lhs == nullptr && rhs == nullptr) {
            return getID() + "(" + printNodeType() + ")" + printNodeDistribution() + "\n";
        }
        string result = getID() + " = " + lhs->getID();
        string oper;
        switch (op) {
            case ASTNode::Operator::ADD:
                oper = "+";
                break;
            case ASTNode::Operator::MINUS:
                oper = "-";
                break;
            case ASTNode::Operator::FFTIMES:
                oper = "*";
                break;
            case ASTNode::Operator::AND:
                oper = "&";
                break;
            case ASTNode::Operator::XOR:
                oper = "^";
                break;
            case ASTNode::Operator::OR:
                oper = "|";
                break;
            /*case ASTNode::Operator::LSL:
                oper = "<<";
                break;
            case ASTNode::Operator::NE:
                oper = "!=";
                break;*/
            case ASTNode::Operator ::RSH:
                oper = ">>";
                break;
            case ASTNode::Operator ::LSH:
                oper = "<<";
                break;
            /*case ASTNode::Operator ::POW2:
                oper = "POW2";
                break;
            case ASTNode::Operator ::POW4:
                oper = "POW4";
                break;
            case ASTNode::Operator ::POW16:
                oper = "POW16";
                break;
            case ASTNode::Operator ::AFFINE:
                oper = "AFFINE";
                break;
            case ASTNode::Operator ::TRCON:
                oper = "RCON";
                break;
            case ASTNode::Operator ::XTIMES:
                oper = "XTIMES";
                break;*/
            case ASTNode::Operator ::NULLOP:
                oper = "";
                break;
            default:
                assert(false);
        }
        if(rhs != nullptr) {
            result += " " + oper + " " + rhs->getID() + "(" + printNodeType() + ")";
        } else {
            result += " " + oper;
        }

        result += "\n";
        return result;
    }

    string prettyPrint2() {
        /*if(this->getNodeType() == NodeType::RANDOM) {
            return " $" + this->getID() + " ";
        }
        else if(this->getNodeType() == NodeType::PUBLIC
                  || this->getNodeType() == NodeType::CONSTANT || this->getNodeType() == NodeType::PARAMETER)
            return " " + this->getID() + " ";
        else if(this->getNodeType() == NodeType::PRIVATE)
            return  " '" + this->getID() + " ";
        else*/
        if(this->getOp() == ASTNode::Operator ::NOT)
            return  "!" + this->getLhs()->prettyPrint2();
        else if(this->getOp() == ASTNode::Operator ::ADD)
            return "(" + this->getLhs()->prettyPrint2()+ "+" + this->getRhs()->prettyPrint2() + ")";
        else if(this->getOp() == ASTNode::Operator ::XOR)
            return "(" + this->getLhs()->prettyPrint2() + "^" + this->getRhs()->prettyPrint2() + ")";
        else if(this->getOp() == ASTNode::Operator ::FFTIMES)
            return "(" + this->getLhs()->prettyPrint2() + "*" + this->getRhs()->prettyPrint2() + ")";
        else if(this->getOp() == ASTNode::Operator ::AND)
            return "(" + this->getLhs()->prettyPrint2() + "&" + this->getRhs()->prettyPrint2() + ")";
        /*else if(this->getOp() == ASTNode::Operator ::POW2)
            return "(POW2 " + this->getLhs()->prettyPrint2() + ")";
        else if(this->getOp() == ASTNode::Operator ::POW4)
            return "(POW4 " + this->getLhs()->prettyPrint2() + ")";
        else if(this->getOp() == ASTNode::Operator ::POW16)
            return "(POW16 " + this->getLhs()->prettyPrint2() + ")";
        else if(this->getOp() == ASTNode::Operator ::AFFINE)
            return "AFFINE" + this->getLhs()->prettyPrint2();
        else if(this->getOp() == ASTNode::Operator ::SBOX)
            return "Sbox" + this->getLhs()->prettyPrint2();
        else if(this->getOp() == ASTNode::Operator ::TABLELUT)
            return "TableLUT" + this->getLhs()->prettyPrint2();
        else if(this->getOp() == ASTNode::Operator ::TABLELUT4)
            return "TableLUT4" + this->getLhs()->prettyPrint2();
        else if(this->getOp() == ASTNode::Operator ::TRCON)
            return "RCON" + this->getLhs()->prettyPrint2();*/
        /*else if(this->getOp() == ASTNode::Operator ::XTIMES)
            return "XTIMES" + this->getLhs()->prettyPrint2();*/

        else
            assert(false);
    }

    int getSsaIndex() const {
        return SSAIndex;
    }

    void setSsaIndex(int ssaIndex) {
        SSAIndex = ssaIndex;
    }



    string getID() {
        if(lineNumber == 0) {
            return nodeName + "_" + to_string(SSAIndex);
        }
        else {
            return nodeName + "_" + to_string(SSAIndex) + "_" + to_string(lineNumber);
        }
    }

    string getIDWithOutLineNumber() {
        return nodeName + "_" + to_string(SSAIndex);
    }

    int getLineNumber() const {
        return lineNumber;
    }

    void setLineNumber(int lineNumber) {
        ThreeAddressNode::lineNumber = lineNumber;
    }

    void addParents(ThreeAddressNodePtr ptr) {
        this->parents.push_back(ptr);
    }

    /*void setParents(vector<ThreeAddressNodeWeakPtr> parents) {
        this->parents = parents;
    }*/

    /*vector<ThreeAddressNodeWeakPtr>& getParents() {
        return this->parents;
    }*/

    void insertSupportV(ThreeAddressNodePtr node) {
        SupportV.insert(node);
    }

    void insertUniqueM(ThreeAddressNodePtr node) {
        UniqueM.insert(node);
    }

    void insertPerfectM(ThreeAddressNodePtr node) {
        PerfectM.insert(node);
    }

    void insertDominant(ThreeAddressNodePtr node) {
        Dominant.insert(node);
    }

    int getSupportVSize() {
        return SupportV.size();
    }

    set<ThreeAddressNodePtr> &getSupportV() {
        return SupportV;
    }

    void setSupportV(const set<ThreeAddressNodePtr> &supportV) {
        SupportV = supportV;
    }

    set<ThreeAddressNodePtr> &getUniqueM(){
        return UniqueM;
    }

    void setUniqueM(const set<ThreeAddressNodePtr> &uniqueM) {
        UniqueM = uniqueM;
    }

    set<ThreeAddressNodePtr> &getPerfectM() {
        return PerfectM;
    }

    void replaceChild(ThreeAddressNodePtr A, ThreeAddressNodePtr B) {

        if(lhs == A) {
            lhs = B;
        } else if(rhs == A) {
            rhs = B;
        }

//        else {
//            assert(rhs == B || lhs == B);
//        }
    }

    void setPerfectM(const set<ThreeAddressNodePtr> &perfectM) {
        PerfectM = perfectM;
    }

    set<ThreeAddressNodePtr> &getDominant() {
        return Dominant;
    }

    void setDominant(const set<ThreeAddressNodePtr> &dominant) {
        Dominant = dominant;
    }

    Distribution getDistribution() const {
        return distribution;
    }

    void setDistribution(Distribution distribution) {
        ThreeAddressNode::distribution = distribution;
    }

    set<ThreeAddressNodePtr > randoms;
    set<ThreeAddressNodePtr > keys;

    void setIndexCall(int i) {
        indexOfCall = i;
    }

    int getIndexCall() {
        return indexOfCall;
    }

    bool operator<(const ThreeAddressNode objNode) const
    {
        return objNode.nodeName > nodeName;
    }

    static void getSupport(ThreeAddressNodePtr node, set<ThreeAddressNodePtr>& res) {
        if(!node)
            return;
        /*if(node->getNodeType() == NodeType::INTERNAL) {
            getSupport(node->getLhs(), res);
            getSupport(node->getRhs(), res);
        }*/
        else {
            res.insert(node);
        }
    }
};

#endif //PLBENCH_THREEADDRESSNODE_H
