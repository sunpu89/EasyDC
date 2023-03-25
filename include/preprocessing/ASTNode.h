//
// Created by Septi on 6/14/2022.
//

#ifndef PLBENCH_ASTNODE_H
#define PLBENCH_ASTNODE_H

#include <iostream>
#include <vector>
#include "json/json.h"
#include <memory>
#include <cassert>

extern std::string cipherName;

class ThreeAddressNode;
class Interpreter;
class Value;
typedef std::shared_ptr<Value> ValuePtr;
typedef std::shared_ptr<ThreeAddressNode> ThreeAddressNodePtr;
typedef std::weak_ptr<ThreeAddressNode> ThreeAddressNodeWeakPtr; // 还不清楚是否有必要一定要加，暂时先加上

namespace ASTNode {
    enum Operator {
        ADD,
        MINUS,
        FFTIMES,
        DIVIDE,
        LSH,
        RSH,
        RLSH,
        RRSH,
        NOT,
        MOD,
        AND,
        OR,
        XOR,
        NEWINDEX, // 用以分割多个index访问
        BOXOP, // 用以表示box permutation，如 sbox<sbox_in>, pbox<pbox_in>
        SYMBOLINDEX, // 当某个数组变量的每个元素都是symbol的时候，且该数组是由NVariavleDeclaration声明时，用于连接symbol表示的元素和索引index
        TOUINT, // 用于连接数组a和标识符b，将数组的所有元素转化成一个整数c，该整数c的标识符也是b
        CALL, // 函数调用，三地址转换时使用
        NULLOP, // 用于表示没有任何操作
        PUSH,
        INDEX,
        BOXINDEX, // 转换三地址时，BOXOP直接操作的数组中的各个元素需要用BOXINDEX来连接起来
    };

    enum Type {
        null,
        uint,
        uint1,
        uint4,
        uint6,
        uint8,
        uint10,
        uint16,
        uint32,
        uint64,
        uint128,
        uint256,
        uint512,
    };


    class Node {
    public:
        virtual std::string getTypeName() const = 0;
        virtual Json::Value jsonGen() const { return Json::Value();}



        // 下面两个纯虚函数用于后续进行三地址转换，for循环展开等
        // virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) {return nullptr;}
        // 将ASTNode转化成Value
        virtual ValuePtr compute(Interpreter& interpreter) {return nullptr;}
    };


    class NExpression : public Node {
    public:
        std::string getTypeName() const {
            return "NExpression";
        }
        Json::Value jsonGen() const {
            Json::Value root;
            root["name"] = getTypeName();
            return root;
        }
    };


    typedef std::shared_ptr<NExpression> NExpressionPtr;
    typedef std::vector<NExpressionPtr> NExpressionList;
    typedef std::shared_ptr<std::vector<NExpressionPtr>> NExpressionListPtr;
    typedef std::vector<NExpressionListPtr> NExpressionListList;
    typedef std::shared_ptr<std::vector<NExpressionListPtr>> NExpressionListListPtr;
    typedef std::shared_ptr<Node> ASTNodePtr;


    class NStatement : public Node {
    public:
        std::string getTypeName() const {
            return "NStatement";
        }
        Json::Value jsonGen() const {
            Json::Value root;
            root["name"] = getTypeName();
            return root;
        }
    };


    typedef std::shared_ptr<NStatement> NStatementPtr;
    typedef std::vector<std::shared_ptr<NStatement>> NStatementList;
    typedef std::shared_ptr<NStatementList> NStatementListPtr;


    class NIdentifier : public NExpression {
    private:
        std::string name;
    public:
        const std::string &getName() const {
            return name;
        }

        void setName(const std::string &name) {
            NIdentifier::name = name;
        }

        NIdentifier(std::string name) : name(name) {
            arraySize = std::make_shared<NExpressionList>();
        }
        bool isType = false;
        bool isArray = false;
        bool isRandom = false;
        bool isParameter = false;

        NExpressionListPtr arraySize;

        std::string getTypeName() const override{
            if(isType) return name;
            return "NIdentifier";
        }
        Json::Value jsonGen() const override {
            Json::Value root;
            root["name"] = getTypeName() + ":" + name + (isArray? "(Array)":"");
            for(auto it = arraySize->begin(); it != arraySize->end(); it++) {
                root["children"].append((*it)->jsonGen());
            }
            return root;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override ;
        virtual ValuePtr compute(Interpreter& interpreter) override;

    };


    typedef std::shared_ptr<NIdentifier> NIdentifierPtr;


    class NArrayIndex : public NExpression {
    private:
        NIdentifierPtr arrayName;
    public:
        NExpressionListPtr dimons;
        NArrayIndex(NIdentifierPtr name)
                : arrayName(name){
            dimons = std::make_shared<NExpressionList>();
        }

        NArrayIndex(NIdentifierPtr name, int size)
                : arrayName(name){
            dimons = std::make_shared<NExpressionList>(size);
        }

        std::string getTypeName() const override {
            return "NArrayIndex";
        }

        NIdentifierPtr getArrayName() {
            return arrayName;
        }

        void setArrayName(NIdentifierPtr arrayname) {
            this->arrayName = arrayname;
        }

        NExpressionPtr getDimonsLast(){
            return dimons->back();
        }

        Json::Value jsonGen() const override {
            Json::Value root;
            root["name"] = getTypeName();
            root["children"].append(arrayName->jsonGen());
            for(auto it = dimons->begin(); it != dimons->end(); it++){
                root["children"].append((*it)->jsonGen());
            }
            return root;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;

    };


    typedef std::shared_ptr<NArrayIndex> NArrayIndexPtr;


    class NAssignment : public NExpression {
    private:
        NIdentifierPtr LHS;
        NExpressionPtr RHS;
    public:
        NAssignment(NIdentifierPtr lhs, NExpressionPtr rhs) : LHS(lhs), RHS(rhs) {}
        std::string getTypeName() const override{
            return "NAssignment";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName();
            root["children"].append(LHS->jsonGen());
            root["children"].append(RHS->jsonGen());
            return root;
        }

        NExpressionPtr getRHS() {
            return RHS;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NFunctionCall : public NExpression {
    private:
        NIdentifierPtr ident;
        NExpressionListPtr arguments;
        int callSite;
    public:
        NFunctionCall(NIdentifierPtr id, NExpressionListPtr argus, int lineNumber) : ident(id), arguments(argus), callSite(lineNumber){}

        std::string getTypeName() const override{
            return "NFunctionCall";
        }

        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName();
            root["children"].append(ident->jsonGen());
            for(auto exp : *arguments)
                root["children"].append(exp->jsonGen());
            return root;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NArrayAssignment : public NExpression {
    private:
        NArrayIndexPtr arrayIndex;
        NExpressionPtr expression;
    public:
        NArrayAssignment(NArrayIndexPtr lhs, NExpressionPtr rhs) : arrayIndex(lhs), expression(rhs) {}
        std::string getTypeName() const override {
            return "NArrayAssignment";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName();
            root["children"].append(arrayIndex->jsonGen());
            root["children"].append(expression->jsonGen());
            return root;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NBinaryOperator : public NExpression {
    private:
        Operator op;
        NExpressionPtr lhs;
        NExpressionPtr rhs;
    public:
        NBinaryOperator(NExpressionPtr lhs, Operator op, NExpressionPtr rhs) : op(op), lhs(lhs), rhs(rhs){}
        std::string getTypeName() const override{
            return "NBinaryOperator";
        }
        Json::Value jsonGen() const override {
            Json::Value root;
            root["name"] = getTypeName();
            root["children"].append(lhs->jsonGen());
            root["children"].append(rhs->jsonGen());
            return root;
        }

        Operator getOp() const {
            return op;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override ;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NUnaryOperator : public NExpression {
    private:
        Operator op;
        NExpressionPtr lhs;
    public:
        NUnaryOperator(NExpressionPtr lhs, Operator op) : op(op), lhs(lhs) {}
        std::string getTypeName() const override{
            return "NUnaryOperator";
        }
        Json::Value jsonGen() const override {
            Json::Value root;
            root["name"] = getTypeName();
            root["children"].append(lhs->jsonGen());
            return root;
        }

        Operator getOp() const {
            return op;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override ;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NInteger : public NExpression {
    private:
        //int value;
        long long value;
    public:
        NInteger(long long val) : value(val) {}
        std::string getTypeName() const override{
            return "NInteger";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName() + std::to_string(value);
            return root;
        }

        long long getValue() const {
            return value;
        }

        void setValue(long long value) {
            NInteger::value = value;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext&) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    typedef std::shared_ptr<NInteger> NIntegerPtr;
    typedef std::vector<NIntegerPtr> NIntegerList;
    typedef std::shared_ptr<NIntegerList> NIntegerListPtr;
    typedef std::vector<NIntegerListPtr> NIntegerListList;
    typedef std::shared_ptr<NIntegerListList> NIntegerListListPtr;


    class NArrayRange : public NExpression {
    private:
        NIdentifierPtr arrayName;
        NIntegerPtr from;
        NIntegerPtr to;
    public:
        NArrayRange(NIdentifierPtr name, NIntegerPtr from, NIntegerPtr to) : arrayName(name), from(from), to(to) {}
        std::string getTypeName() const override{
            return "NArrayRange";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName() ;
            root["children"].append(arrayName->jsonGen());
            root["children"].append(from->jsonGen());
            root["children"].append(to->jsonGen());
            return root;
        }

        NIntegerPtr getFrom() {
            return from;
        }

        NIntegerPtr getTo() {
            return to;
        }

        NIdentifierPtr getIdentifier() {
            return arrayName;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override;
    };


    typedef std::shared_ptr<NArrayRange> NArrayRangePtr;


    // 以下两个类分别是表达式View和touint的分析类。
    class NViewArray : public NExpression {
    private:
        NIdentifierPtr arrayName;
        NExpressionPtr from;
        NExpressionPtr to;
    public:
        NViewArray(NIdentifierPtr name, NExpressionPtr from, NExpressionPtr to) : arrayName(name), from(from), to(to) {}
        std::string getTypeName() const override{
            return "NViewArray";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName() ;
            root["children"].append(arrayName->jsonGen());
            root["children"].append(from->jsonGen());
            root["children"].append(to->jsonGen());
            return root;
        }

        NIdentifierPtr getIdentifier() {
            return arrayName;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NToUint : public NExpression {
    private:
        NIdentifierPtr arrayName;
        NExpressionListPtr expressionListPtr;
    public:
        NToUint(NExpressionListPtr expressionListPtr) : expressionListPtr(expressionListPtr) {}

        NToUint(NIdentifierPtr name) : arrayName(name) {}

        std::string getTypeName() const override{
            return "NToUint";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName() ;
            if (arrayName != nullptr)
                root["children"].append(arrayName->jsonGen());
            if (expressionListPtr != nullptr) {
                for (auto exp: *expressionListPtr)
                    root["children"].append(exp->jsonGen());
            }
            return root;
        }

        NIdentifierPtr getIdentifier() {
            return arrayName;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    // 该类用于分析 sbox进行的substitution和pbox进行的linear transformation
    class NBoxOperation : public NExpression {
    private:
        NIdentifierPtr boxname;
        NExpressionPtr expressionPtr;
    public:
        NBoxOperation(NIdentifierPtr name, NExpressionPtr expressionPtr) : boxname(name), expressionPtr(expressionPtr) {}

        std::string getTypeName() const override{
            return "NBoxOperation";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName() ;
            root["children"].append(boxname->jsonGen());
            root["children"].append(expressionPtr->jsonGen());
            return root;
        }

        NIdentifierPtr getIdentifier() {
            return boxname;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NVariableDeclaration : public NStatement {
    private:
        const NIdentifierPtr type;
        NIdentifierPtr id;
        NExpressionPtr assignmentExpr = nullptr;
    public:
        bool isParameter = false;
        NVariableDeclaration(){}

        const NIdentifierPtr &getId() {
            return id;
        }

        NVariableDeclaration(const std::shared_ptr<NIdentifier> type, std::shared_ptr<NIdentifier> id,
                             std::shared_ptr<NExpression> assignmentExpr = NULL)
                : type(type), id(id), assignmentExpr(assignmentExpr) {
//            cout << "isArray = " << type->isArray << endl;
            assert(type->isType);
        }

        std::string getTypeName() const override {
            return "NVariableDeclaration";
        }

        std::string getMyType() {
            return type->getName();
        }

        NExpressionPtr getAssignmentExpr() {
            return assignmentExpr;
        }

        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName() ;
            root["children"].append(type->jsonGen());
            root["children"].append(id->jsonGen());
            if(assignmentExpr != nullptr)
                root["children"].append(assignmentExpr->jsonGen());
            return root;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    typedef std::shared_ptr<NVariableDeclaration> NVariableDeclarationPtr;
    typedef std::vector<NVariableDeclarationPtr> NVariableList;
    typedef std::shared_ptr<NVariableList> NVariableListPtr;


    class NArrayInitialization: public NStatement {
    private:
        NVariableDeclarationPtr declaration;
        NExpressionListPtr expressionList = std::make_shared<NExpressionList>();

        // 下面这个成员变量用于数组之间直接进行操作，如 uint1[32] a = b ^ c; 其中b和c都是数组类型变量。
        // NExpressionPtr expressionPtr; // 现在我们在后续处理时再进行转化，先不用这个方法
    public:
        NArrayInitialization() {}
        NArrayInitialization(std::shared_ptr<NVariableDeclaration> dec, std::shared_ptr<NExpressionList> list)
                : declaration(dec), expressionList(list) {
        }

        /*NArrayInitialization(std::shared_ptr<NVariableDeclaration> dec, std::shared_ptr<NExpression> exp)
                : declaration(dec), expressionPtr(exp) {
        }*/


        std::string getTypeName() const override{
            return "NArrayInitialization";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName() ;
            root["children"].append(declaration->jsonGen());
            if (expressionList != nullptr) {
                for (auto exp: *expressionList)
                    root["children"].append(exp->jsonGen());
            }
            /*if (expressionPtr != nullptr) {
                root["children"].append(expressionPtr->jsonGen());
            }*/
            return root;
        }

        //virtual ThreeAddressNode* threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NSboxInitialization: public NStatement {
    private:
        NVariableDeclarationPtr declaration;
        NIntegerListPtr nIntegerListPtr = std::make_shared<NIntegerList>();
    public:
        NSboxInitialization() {}
        NSboxInitialization(std::shared_ptr<NVariableDeclaration> dec, std::shared_ptr<NIntegerList> list)
                : declaration(dec), nIntegerListPtr(list) {

        }
        std::string getTypeName() const override{
            return "NSboxInitialization";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = "Sbox:" + getTypeName() ;
            root["children"].append(declaration->jsonGen());
            for(auto exp : *nIntegerListPtr)
                root["children"].append(exp->jsonGen());
            return root;
        }

        //virtual ThreeAddressNode* threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NPboxInitialization: public NStatement {
    private:
        NVariableDeclarationPtr declaration;
        NIntegerListPtr nIntegerListPtr = std::make_shared<NIntegerList>();
    public:
        NPboxInitialization() {}
        NPboxInitialization(std::shared_ptr<NVariableDeclaration> dec, std::shared_ptr<NIntegerList> list)
                : declaration(dec), nIntegerListPtr(list) {

        }
        std::string getTypeName() const override{
            return "NPboxInitialization";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = "Pbox:" + getTypeName() ;
            root["children"].append(declaration->jsonGen());
            for(auto exp : *nIntegerListPtr)
                root["children"].append(exp->jsonGen());
            return root;
        }

        //virtual ThreeAddressNode* threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NPboxmInitialization: public NStatement {
    private:
        NVariableDeclarationPtr declaration;
        NIntegerListListPtr nIntegerListListPtr;
    public:
        NPboxmInitialization() {}
        NPboxmInitialization(std::shared_ptr<NVariableDeclaration> dec, std::shared_ptr<NIntegerListList> list)
                : declaration(dec), nIntegerListListPtr(list) {

        }
        std::string getTypeName() const override{
            return "NPboxmInitialization";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = "Pboxm:" + getTypeName() ;
            root["children"].append(declaration->jsonGen());
            for(auto integerList : *nIntegerListListPtr)
                for (auto integer : *integerList)
                    root["children"].append(integer->jsonGen());
            return root;
        }

        //virtual ThreeAddressNode* threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NFfmInitialization: public NStatement {
    private:
        NVariableDeclarationPtr declaration;
        NIntegerListListPtr nIntegerListListPtr;
    public:
        NFfmInitialization() {}
        NFfmInitialization(std::shared_ptr<NVariableDeclaration> dec, std::shared_ptr<NIntegerListList> list)
                : declaration(dec), nIntegerListListPtr(list) {

        }
        std::string getTypeName() const override{
            return "NFfmInitialization";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = "Ffm:" + getTypeName() ;
            root["children"].append(declaration->jsonGen());
            for(auto integerList : *nIntegerListListPtr)
                for (auto integer : *integerList)
                    root["children"].append(integer->jsonGen());
            return root;
        }

        //virtual ThreeAddressNode* threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NBlock : public NStatement {
    private:
        NStatementListPtr stmt_list;
    public:
        const NStatementListPtr &getStmtList() const {
            return stmt_list;
        }

        void setStmtList(const NStatementListPtr &stmtList) {
            stmt_list = stmtList;
        }

        NBlock() {
            stmt_list = std::make_shared<NStatementList>();
        }
        std::string getTypeName() const override{
            return "NBlock";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName() ;
            for(auto exp : *stmt_list)
                root["children"].append(exp->jsonGen());
            return root;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;

    };


    typedef std::shared_ptr<NBlock> NBlockPtr;


    class NFunctionDeclaration : public NStatement {
    private:
        NIdentifierPtr type;
        NIdentifierPtr id;
        NVariableListPtr var_list;
        NBlockPtr block;
    public:
        NFunctionDeclaration(NIdentifierPtr type, NIdentifierPtr id,
                             NVariableListPtr var_list, NBlockPtr block) : type(type), id(id), var_list(var_list), block(block) {}
        std::string getTypeName() const override{
            return "NFunctionDeclaration";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName() ;
            root["children"].append(type->jsonGen());
            root["children"].append(id->jsonGen());
            for(auto exp : *var_list)
                root["children"].append(exp->jsonGen());
            root["children"].append(block->jsonGen());
            return root;
        }
        const NIdentifierPtr &getId() const {
            return id;
        }

        void setId(const NIdentifierPtr &id) {
            NFunctionDeclaration::id = id;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context)override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NRoundFunctionDeclaration : public NStatement {
    private:
        NIdentifierPtr type;
        NIdentifierPtr id;
        NVariableDeclarationPtr round;
        NVariableDeclarationPtr sk;
        NVariableDeclarationPtr p;
        NBlockPtr block;
    public:
        NRoundFunctionDeclaration(NIdentifierPtr type, NIdentifierPtr id, std::shared_ptr<NVariableDeclaration> round,
                                  std::shared_ptr<NVariableDeclaration> sk, std::shared_ptr<NVariableDeclaration> p, NBlockPtr block) :
                              type(type), id(id), round(round), sk(sk), p(p), block(block) {
            round->isParameter = true;
            sk->isParameter = true;
            p->isParameter = true;
        }
        std::string getTypeName() const override{
            return "NRoundFunctionDeclaration";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName() ;
            root["children"].append(type->jsonGen());
            root["children"].append(id->jsonGen());
            root["children"].append(round->jsonGen());
            root["children"].append(sk->jsonGen());
            root["children"].append(p->jsonGen());
            root["children"].append(block->jsonGen());
            return root;
        }
        const NIdentifierPtr &getId() const {
            return id;
        }

        void setId(const NIdentifierPtr &id) {
            NRoundFunctionDeclaration::id = id;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context)override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NSboxFunctionDeclaration : public NStatement {
    private:
        NIdentifierPtr type;
        NIdentifierPtr id;
        NVariableDeclarationPtr input;
        NBlockPtr block;
    public:
        NSboxFunctionDeclaration(NIdentifierPtr type, NIdentifierPtr id, NVariableDeclarationPtr input,
                                  NBlockPtr block) :
                type(type), id(id), input(input), block(block) {}
        std::string getTypeName() const override{
            return "NSboxFunctionDeclaration";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName() ;
            root["children"].append(type->jsonGen());
            root["children"].append(id->jsonGen());
            root["children"].append(input->jsonGen());
            root["children"].append(block->jsonGen());
            return root;
        }
        const NIdentifierPtr &getId() const {
            return id;
        }

        void setId(const NIdentifierPtr &id) {
            NSboxFunctionDeclaration::id = id;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context)override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NCipherFunctionDeclaration : public NStatement {
    private:
        NIdentifierPtr type;
        NIdentifierPtr id;
        NVariableDeclarationPtr k;
        NVariableDeclarationPtr p;
        NBlockPtr block;
    public:
        NCipherFunctionDeclaration(NIdentifierPtr type, NIdentifierPtr id, NVariableDeclarationPtr k,
                                 NVariableDeclarationPtr p, NBlockPtr block) :
                type(type), id(id), k(k), p(p), block(block) {
            k->isParameter = true;
            p->isParameter = true;
        }

        std::string getTypeName() const override{
            return "NCipherFunctionDeclaration";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName() ;
            root["children"].append(type->jsonGen());
            root["children"].append(id->jsonGen());
            root["children"].append(k->jsonGen());
            root["children"].append(p->jsonGen());
            root["children"].append(block->jsonGen());
            return root;
        }
        const NIdentifierPtr &getId() const {
            return id;
        }

        void setId(const NIdentifierPtr &id) {
            NCipherFunctionDeclaration::id = id;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context)override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NExpressionStatement : public NStatement {
    private:
        NExpressionPtr expr;
    public:
        NExpressionStatement(NExpressionPtr expr) : expr(expr) {}
        std::string getTypeName() const override{
            return "NExpressionStatement";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName() ;
            root["children"].append(expr->jsonGen());
            return root;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NReturnStatement : public NStatement {
    private:
        NExpressionPtr expr;
    public:
        NReturnStatement(NExpressionPtr expr) : expr(expr) {}
        std::string getTypeName() const override{
            return "NReturnStatement";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName() ;
            root["children"].append(expr->jsonGen());
            return root;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    // ForStatement Revised: 这里将forstatemt
    // 由 for( i = 0; i <= 10; i = i + 1) {} 改为 for (i from 0 to 10) {}
    /*class NForStatement : public NStatement {
    private:
        NExpressionPtr init;
        NExpressionPtr condition;
        NExpressionPtr increase;
        NBlockPtr block;
        int from;
        int to;
        std::string name;
    public:
        NForStatement(const ASTNode::NExpressionPtr &init, const ASTNode::NExpressionPtr &condition,
                      const ASTNode::NExpressionPtr &increase, const ASTNode::NBlockPtr &block) : init(
                init), condition(condition), increase(increase), block(block) {}
        std::string getTypeName() const override{
            return "NForStatement";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName() ;
            root["children"].append(init->jsonGen());
            root["children"].append(condition->jsonGen());
            root["children"].append(increase->jsonGen());
            root["children"].append(block->jsonGen());
            return root;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };*/

    class NForStatement : public NStatement {
    private:
        NIdentifierPtr ident;
        NIntegerPtr from;
        NIntegerPtr to;
        NBlockPtr block;
    public:
        NForStatement(const ASTNode::NIdentifierPtr &ident, const ASTNode::NIntegerPtr &from,
                      const ASTNode::NIntegerPtr &to, const ASTNode::NBlockPtr &block) : ident(
                ident), from(from), to(to), block(block) {}
        std::string getTypeName() const override{
            return "NForStatement";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName() ;
            root["children"].append(ident->jsonGen());
            root["children"].append(from->jsonGen());
            root["children"].append(to->jsonGen());
            root["children"].append(block->jsonGen());
            return root;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;
    };


    class NCipherNameDeclaration : public NStatement {
    private:
        std::string name;
    public:
        NCipherNameDeclaration(std::string name) : name(name) {
            cipherName = name;
        }
        std::string getTypeName() const override{
            return "NCipherNameDeclaration";
        }
        Json::Value jsonGen() const override{
            Json::Value root;
            root["name"] = getTypeName() + ":" + name;
            return root;
        }

        //virtual ThreeAddressNodePtr threeAddCodeGen(IRCodeGenContext& context) override;
        virtual ValuePtr compute(Interpreter& interpreter) override;

        // 下面这个函数只是为了测试，测试是否在ASTNode.cpp文件中声明函数主体并不影响程序运行。
        // used for typing system
        int typing(int a);
    };


    // STOP HERE
    /*
     * 1. r_fn //
     * 2. s_fn //
     * 3. fn //
     * 4. pbox //
     * 5. sbox //
     * 6. pboxm //
     * 7. ffm //
     * 8. View
     * 9. touint
     * 10.
     * */

}


#endif //PLBENCH_ASTNODE_H
