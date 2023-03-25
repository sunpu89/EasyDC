//
// Created by Septi on 7/5/2022.
//

#ifndef PLBENCH_INLINER_H
#define PLBENCH_INLINER_H

#include "ProcedureH.h"

class Inliner {
private:
    vector<ProcedureHPtr> procedures;
    vector<ProcedureHPtr> inlinedProcedure;
    map<string, ProcedureHPtr> nameToInlinedProcedure;

public:
    Inliner(const vector<ProcedureHPtr>& procedures) : procedures(procedures){}

    void inlineProcedures();
    vector<ThreeAddressNodePtr> instantitateBlockByComputation(ProcedureHPtr proc, ThreeAddressNodePtr callInstruction,map<string, ThreeAddressNodePtr>& saved, map<ThreeAddressNodePtr, ThreeAddressNodePtr>& savedForFunction,
                                                               map<ThreeAddressNodePtr, ThreeAddressNodePtr>& formalToActual, string path, set<ThreeAddressNodePtr>& backup );


    void instantitateProcedure(const ProcedureHPtr proc, const vector<ActualPara>& actualArguments, const ThreeAddressNodePtr& callInstruction,
                               vector<ThreeAddressNodePtr>& newBlock, vector<ThreeAddressNodePtr>& newReturn,
                               map<string, ThreeAddressNodePtr>& saved, map<ThreeAddressNodePtr, ThreeAddressNodePtr>& savedForFunction, const map<ThreeAddressNodePtr, string>& callToNodeName);

    ThreeAddressNodePtr instantitateNode(ProcedureHPtr proc, string basename, ThreeAddressNodePtr node, map<ThreeAddressNodePtr, ThreeAddressNodePtr>& formalToActual, map<string, ThreeAddressNodePtr>& saved, map<ThreeAddressNodePtr, ThreeAddressNodePtr >& savedForFunction, string path, set<ThreeAddressNodePtr>& randomBackup);

};

#endif //PLBENCH_INLINER_H
