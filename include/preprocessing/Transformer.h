//
// Created by Septi on 6/27/2022.
//

#ifndef PLBENCH_TRANSFORMER_H
#define PLBENCH_TRANSFORMER_H

#include "Value.h"
#include "list"
#include "ProcedureH.h"

class Transformer {
private:
    vector<ProcValuePtr> procedures;
    vector<ProcedureHPtr> procedureHs;

    // 在transformArrayValue2oneTAN中作为counter使用
    int leftoverCounter = 0;
public:
    const vector<ProcedureHPtr> &getProcedureHs() const;

private:
    map<string, ProcedureHPtr> nameToProc;

public:
    Transformer(const vector<ProcValuePtr>& procedures) {
        this->procedures = procedures;
    }

    void transformProcedures();

    /*ThreeAddressNodePtr
    transformRandom(RandomValuePtr randomValuePtr, map<ValuePtr, ThreeAddressNodePtr> &saved,
                    map<string, int> &nameCount);*/

    ThreeAddressNodePtr
    transformInternalBinForShare(InternalBinValuePtr internalBinValuePtr, map<ValuePtr, ThreeAddressNodePtr> &saved,
                                 map<string, int> &nameCount);

    ThreeAddressNodePtr
    transformInternalBin(InternalBinValuePtr internalBinValuePtr, map<ValuePtr, ThreeAddressNodePtr> &saved,
                         map<string, int> &nameCount);

    vector<ThreeAddressNodePtr>
    transformInternalBinVec(InternalBinValuePtr internalBinValuePtr, map<ValuePtr, ThreeAddressNodePtr> &saved,
                         map<string, int> &nameCount);

    ThreeAddressNodePtr
    transformInternalUn(InternalUnValuePtr internalUnValuePtr, map<ValuePtr, ThreeAddressNodePtr> &saved,
                        map<string, int> &nameCount);

    vector<ThreeAddressNodePtr>
    transformTouint(InternalUnValuePtr internalUnValuePtr, map<ValuePtr, ThreeAddressNodePtr> &saved,
                        map<string, int> &nameCount);


    vector<ThreeAddressNodePtr>
    transformArrayValue(ArrayValuePtr arrayValuePtr, map<ValuePtr, ThreeAddressNodePtr> &saved,
                        map<string, int> &nameCount, bool&);

    ThreeAddressNodePtr
    transformArrayValue2oneTAN(ArrayValuePtr arrayValuePtr, map<ValuePtr, ThreeAddressNodePtr> &saved,
                        map<string, int> &nameCount, bool&);

    // added by sunpu, used for transforming the value of arrayvalueindex which has the symbol value
    ThreeAddressNodePtr
    transformArrayValueIndex(ArrayValueIndex arrayValueIndex, map<ValuePtr, ThreeAddressNodePtr> &saved,
                             map<string, int> &nameCount, bool &isSimple);

    ThreeAddressNodePtr
    transformBoxValue(BoxValuePtr boxValuePtr, map<ValuePtr, ThreeAddressNodePtr> &saved, bool &isSimple);

    ProcedureHPtr transform(ProcValuePtr procValuePtr);

    string getCount(string name, map<string, int> &nameCount);

    static NodeType getNodeType(ASTNode::Type type);

    static int getNodeTypeSize(NodeType nodeType);
};


#endif //PLBENCH_TRANSFORMER_H
