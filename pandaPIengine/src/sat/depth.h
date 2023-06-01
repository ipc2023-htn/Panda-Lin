//
// Created by lst19 on 4/16/2023.
//

#ifndef PANDAPIENGINE_DEPTH_H
#define PANDAPIENGINE_DEPTH_H
#include "../Model.h"
#include "distribution.h"
#include "fstream"
#include <string.h>

// bool isMethodPrecondition(string name) {
//     string prefix = "__method_precondition";
//     string noop = "__noop";
//     auto matching = mismatch(prefix.begin(), prefix.end(), name.begin());
//     if (matching.first == prefix.end()) return true;
//     auto matchingNoop = mismatch(noop.begin(), noop.end(), name.begin());
//     if (matchingNoop.first == noop.end()) return true;
//     return false;
// }

class Depth {
public:
    Depth(Model *htn, int length);
    int get(int task, int length) {return this->depth[task][length];}
    int get() {return this->depth[this->htn->initialTask][this->maxLength];}
private:
    Model *htn;
    int maxLength;
    vector<Distributions*> distributions;
    vector<vector<int>> depth;
    void depthPerSCC(int length, int scc);
    void update(int length, int scc, bool allowEmptiness);
};
#endif //PANDAPIENGINE_DEPTH_H
