#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <cassert>
#include <set>
#include <iostream>
#include "IR.h"
// using std::map;
// using std::set;
// using std::string;
// using std::stringstream;
// using std::to_string;
// using std::vector;
using namespace std;

template <typename T> set<T> set_union(const set<T> &a, const set<T> &b) {
    set<T> c;
    set_union(a.begin(), a.end(), b.begin(), b.end(),
              insert_iterator<set<T>>(c, c.begin()));
    return c;
}

template <typename T> set<T> set_difference(const set<T> &a, const set<T> &b) {
    set<T> c;
    set_difference(a.begin(), a.end(), b.begin(), b.end(),
                   insert_iterator<set<T>>(c, c.begin()));
    return c;
}

vector<IRCode> algebraOptimize(const vector<IRCode> &ircodes) {
    vector<IRCode> result;
    for (const auto &ircode : ircodes) {
        switch (ircode.op) {
        case IROperator::ADD:
            if (ircode.op1 == "0") {
                result.push_back(
                    IRCode(IROperator::MOV, ircode.op2, "", ircode.dst));
            } else if (ircode.op2 == "0") {
                result.push_back(
                    IRCode(IROperator::MOV, ircode.op1, "", ircode.dst));
            } else {
                result.push_back(ircode);
            }
            break;
        case IROperator::SUB:
            if (ircode.op1 == ircode.op2) {
                result.push_back(IRCode(IROperator::MOV, "0", "", ircode.dst));
            } else if (ircode.op2 == "0") {
                result.push_back(
                    IRCode(IROperator::MOV, ircode.op1, "", ircode.dst));
            } else {
                result.push_back(ircode);
            }
            break;
        case IROperator::MUL:
            if (ircode.op1 == "1") {
                result.push_back(
                    IRCode(IROperator::MOV, ircode.op2, "", ircode.dst));
            } else if (ircode.op2 == "1") {
                result.push_back(
                    IRCode(IROperator::MOV, ircode.op1, "", ircode.dst));
            } else if (ircode.op1 == "0" || ircode.op2 == "0") {
                result.push_back(IRCode(IROperator::MOV, "0", "", ircode.dst));
            } else {
                result.push_back(ircode);
            }
            break;
        case IROperator::DIV:
            if (ircode.op1 == ircode.op2) {
                result.push_back(IRCode(IROperator::MOV, "1", "", ircode.dst));
            } else if (ircode.op2 == "1") {
                result.push_back(
                    IRCode(IROperator::MOV, ircode.op1, "", ircode.dst));
            } else if (ircode.op1 == "0" || ircode.op2 == "0") {
                result.push_back(IRCode(IROperator::MOV, "0", "", ircode.dst));
            } else {
                result.push_back(ircode);
            }
            break;
        default:
            result.push_back(ircode);
            break;
        }
    }
    return result;
}

struct ReachFlow {};

void registerAssign() {}
