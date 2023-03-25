#include <iostream>
#include "json/json.h"
#include "ASTNode.h"
#include "Interpreter.h"
#include "Transformer.h"
#include "WwMILP.h"
#include "BwMILP.h"

extern int yyparse();
extern int yydebug;
extern FILE *yyin;
extern int yylineno;
extern std::shared_ptr<ASTNode::NBlock> programRoot;

map<std::string, std::vector<int>> allBox = {};
map<std::string, std::vector<int>> pboxM = {};
map<std::string, int> pboxMSize = {};
map<std::string, std::vector<int>> Ffm = {};
std::string cipherName;

void SboxModelingMGR(std::vector<std::string> params);
void MILPMGR(std::vector<std::string> params);

int main(int argc, const char* argv[]) {
    std::vector<std::string> params;
    for (int i = 1; i < argc; ++i) {
        params.emplace_back(argv[i]);
    }
    // sbox modeling
    if (argc == 5) {
        SboxModelingMGR(params);
    } else if (argc >= 6) {
        MILPMGR(params);
    }
    // read setup from file
    else {
        params.clear();
        std::string path = "../parametersSboxDemo.txt";
        std::ifstream file;
        file.open(path);
        std::string model, line;
        std::string whiteSpaces = " \n\r\t\f\v";
        while (getline(file, line)) {
            // trim and save parameters
            size_t first_non_space = line.find_first_not_of(whiteSpaces);
            line.erase(0, first_non_space);
            size_t last_non_space = line.find_last_not_of(whiteSpaces);
            line.erase(last_non_space + 1);
            params.push_back(line);
        }
        file.close();
        int paramNum = params.size();
        if (paramNum == 4) {
            SboxModelingMGR(params);
        } else if (paramNum >= 5) {
            MILPMGR(params);
        } else
            assert(false);
    }
    return 0;
}

void SboxModelingMGR(std::vector<std::string> params) {
    /*
     * argv[1] -> sboxName;
     * argv[2] -> sbox;    e.g.   "1,2,3,4,5,6,7,8,9,...,16"
     * argv[3] -> modeling mode;
     *            mode = "AS" -> modeling differential propagation
     *            mode = "DC" -> modeling differential propagation with probability
     * argv[4] -> reduction methods;
     *            Note : here we only recommend the value of redMd takes 1 to 6;
     *            redMd = 1 -> greedy algorithm
     *            redMd = 2 -> sub_milp
     *            redMd = 3 -> convex_hull_tech
     *            redMd = 4 -> logic_cond
     *            redMd = 5 -> comb233
     *            redMd = 6 -> superball
     *            redMd = 7 -> read inequalities from external used to extract the results of method of Udovenko (Hellman) or CNF
     *            redMd = 8 -> read inequalities from external used to extract the results of method of Udovenko (Hellman) or CNF
     * */
    std::string sboxName = params[0];
    std::vector<string> sboxStr = utilities::split(params[1], ",");
    std::vector<int> sbox;
    for (const auto &ele: sboxStr) { sbox.push_back(std::stoi(ele)); };
    std::string mode = params[2];
    int redMd = std::stoi(params[3]);
    SboxM sboxM(sboxName, sbox, mode);
    std::vector<std::vector<int>> redResults = Red::reduction(redMd, sboxM);
}

void MILPMGR(std::vector<std::string> params) {
    /*
     * argv[1] -> the number of parameters
     * argv[2] -> EasyBC implementation file path;
     * argv[3] -> word-wise or bit-wise or extended bit-wise
     *            techChoose = "w" -> word-wise
     *            techChoose = "b" -> bit-wise
     *            techChoose = "d" -> extended bit-wise
     * argv[4] -> modeling mode for S-boxes, same as the "case 1";
     * argv[5] -> reduction methods for S-boxes; same as the "case 1";
     *
     * optional parameters :
     * argv[6]/argv[8]/argv[10]/argv[12] -> startRound or allRounds or timer(second) or threadsNum
     * argv[7]/argv[9]/argv[11]/argv[13] -> startRound or allRounds or timer(second) or threadsNum
     * */
    std::string filePath = params[1];
    yyin = fopen(filePath.c_str(), "r");
    if (!yyin) {
        std::cout << "Wrong Path : \n" << filePath << std::endl;
        assert(false);
    }
    yydebug = 0;
    yylineno = 1;
    std::vector<ProcValuePtr> res;
    Interpreter interpreter;
    if (!yyparse()) {
        std::cout << "Parsing complete\n" << std::endl;
    } else {
        std::cout << "Hint : wrong syntax at line " << yylineno << std::endl;
        assert(false);
    }
    interpreter.generateCode(*programRoot);
    res = interpreter.getProcs();

    Transformer transformer(res);
    transformer.transformProcedures();

    // extract optional parameters
    int startRound = 1, allRounds = 32, timer = 3600 * 12, threadsNum = 16;
    for (int i = 5; i < std::stoi(params[0]); i = i + 2) {
        if (params[i] == "startRound") {
            startRound = std::stoi(params[i + 1]);
        } else if (params[i] == "allRounds") {
            allRounds = std::stoi(params[i + 1]);
        } else if (params[i] == "timer") {
            timer = std::stoi(params[i + 1]);
        } else if (params[i] == "threadsNum") {
            threadsNum = std::stoi(params[i + 1]);
        } else
            assert(false);
    }

    // word-wise
    if (params[2] == "w") {
        WwMILP wwMilp(transformer.getProcedureHs());
        wwMilp.setStartRound(startRound);
        wwMilp.setAllRound(allRounds);
        wwMilp.setTimer(timer);
        wwMilp.setThreadNum(threadsNum);
        wwMilp.MILP();
    }
    // bit-wise or extended bit-wise
    else if (params[2] == "b" or params[2] == "d") {
        if (params[2] == "b" and params[3] != "AS") {
            std::cout << "Bit-wise approach should be used to compute the minimal number of active S-boxes."
                      << std::endl;
            assert(false);
        } else if (params[2] == "d" and params[3] != "DC") {
            std::cout
                    << "Extended bit-wise approach should be used to compute the maximum probability of differential characteristics."
                    << std::endl;
            assert(false);
        }
        BwMILP bwMilp(transformer.getProcedureHs(), params[3], std::stoi(params[4]));
        bwMilp.setStartRound(startRound);
        bwMilp.setAllRound(allRounds);
        bwMilp.setTimer(timer);
        bwMilp.setThreadNum(threadsNum);
        bwMilp.MILP();
    } else
        assert(false);
}