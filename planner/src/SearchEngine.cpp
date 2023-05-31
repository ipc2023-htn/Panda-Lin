//============================================================================
// Name        : SearchEngine.cpp
// Author      : Daniel Höller
// Version     :
// Copyright   : 
// Description : Search Engine for Progression HTN Planning
//============================================================================

#include "flags.h" // defines flags

#include <iostream>
#include <stdlib.h>
#include <getopt.h>
#include <unordered_set>
#include <landmarks/lmExtraction/LmCausal.h>
#include <landmarks/lmExtraction/LMsInAndOrGraphs.h>
#include <fringes/OneQueueWAStarFringe.h>
#include "./search/PriorityQueueSearch.h"

#include "Debug.h"
#include "Model.h"

#include "linearizer.h"

#include "interactivePlanner.h"
#include "sat/sat_planner.h"
#include "Invariants.h"

#include "symbolic_search/automaton.h"

#include "translation/translationController.h"

#include "intDataStructures/IntPairHeap.h"
#include "intDataStructures/bIntSet.h"

#include "heuristics/hhSimple.h"

#include "heuristics/rcHeuristics/RCModelFactory.h"
#include "heuristics/landmarks/lmExtraction/LmFdConnector.h"
#include "heuristics/landmarks/hhLMCount.h"
#ifndef CMAKE_NO_ILP
#include "heuristics/dofHeuristics/hhStatisticsCollector.h"
#endif
#include "VisitedList.h"

#include "cmdline.h"

using namespace std;
using namespace progression;

vector<string> parse_list_of_strings(istringstream & ss){
	vector<string> strings;
	while (true){
		if (ss.eof()) break;
		string x; ss >> x;
		strings.push_back(x);
	}

	return strings;
}

vector<string> parse_list_of_strings(string & line){
	istringstream ss (line);
	return parse_list_of_strings(ss);
}


pair<string,map<string,string>> parse_heuristic_with_arguments_from_braced_expression(string str){
	string heuristic = "";
	size_t pos= 0;
	while (pos < str.size() && str[pos] != '('){
   		heuristic += str[pos];
		pos++;
	}
	
	map<string,string> arguments;

   	if (pos != str.size()){
		string argument_string = str.substr(pos+1,str.size() - pos - 2);
		replace(argument_string.begin(), argument_string.end(), ';', ' ');
		vector<string> args = parse_list_of_strings(argument_string);

		int position = 1;
		for (string arg : args){
			replace(arg.begin(), arg.end(), '=', ' ');
			vector<string> argElems = parse_list_of_strings(arg);
			if (argElems.size() == 1)
				arguments["arg" + to_string(position)] = argElems[0];
			else if (argElems.size() == 2)	
				arguments[argElems[0]] = argElems[1];
			else{
				cout << "option " << arg << " has more than one equals sign ..." << endl;
				exit(1);
			}
			position++;
		}
	} // else there are none

	return make_pair(heuristic,arguments);
}


enum planningAlgorithm{
	PROGRESSION,SAT,BDD,INTERACTIVE,TRANSLATION
};


void speed_test();


void printHeuristicHelp(){
	cout << "Each heuristic is to be specified as a single string." << endl;
	cout << "The following heuristics are available" << endl;
	cout << "" << endl;
	cout << "  zero" << endl;
	cout << "  modDepth" << endl;
	cout << "  mixedModDepth" << endl;
	cout << "  cost" << endl;
	cout << "  rc2" << endl;
	cout << "  dof" << endl;
	cout << "" << endl;
	cout << "Arguments are given to the heuristics in normal braces. E.g. use cost(invert) to pass the argument 'invert' to the heuristic 'cost'." << endl;
	cout << "We explain below which arguments (like 'invert') exist for each heuristic. Multiple arguments must be separated by semicolons. " << endl;
	cout << "By default the arguments can just be given in order. It is also possible to provide them using the key=value syntax. The name" << endl;
	cout << "of the argument is given in braces" << endl;
	cout << "" << endl;
	cout << "- zero is a heuristic that returns always zero." << endl;
	cout << "- cost is a heuristic that return the action costs of the actions taken leading to the current search node" << endl;
	cout << "  (not including actions present in the current (remaining) task network)" << endl;
	cout << "- modDepth and mixedModDepth are as explained in the planner helptext, as possible values for -g" << endl;
	cout << "- The modDepth, mixedModDepth, and cost heuristics all have only one optional argument \"invert\" (or invert=true)." << endl;
	cout << "  If provided the heuristic value will be multiplied with -1." << endl;
	cout << "" << endl;
	cout << "- The rc2 heuristic has the following arguments:" << endl;
	cout << "  1. The inner heuristic (key: h). If you don't provide one, this will default to ff. You may choose one of" << endl;
	cout << "    - ff (the FF heuristic)" << endl;
	cout << "    - add (the additive heuristic)" << endl;
	cout << "    - lmc (the LM-cut heuristic)" << endl;
	cout << "    - filter (0 if the HTN goal is reachable under delete relaxation, infinity otherwise)" << endl;
	cout << "  2. What to estimate (key: est). There are three possible values. The default is distance" << endl;
	cout << "    - distance (estimate number of actions and methods to apply)" << endl;
	cout << "    - cost (estimate cost of actions to apply)" << endl;
	cout << "    - mixed (estimate cost of actions plus number of methods to apply)" << endl;
	cout << "  3. Correct Task Count (key: taskcount). This option is on by default." << endl;
	cout << "     If on, the heuristic is improved by counting the minimally implied cost/count " << endl;
	cout << "     of actions that occur more than once in the current task network." << endl;
	cout << "     To turn it off, pass no as the third argument" << endl;
	cout << "" << endl;
	cout << "- The dof (delete- and order-free relaxation) heuristic requires that pandaPIengine is compiled with CPLEX support." << endl;
	cout << "  It has the following 8 arguments." << endl;
	cout << "  1. Type (key: type). Default is ilp. Can be set to lp. Determines whether the problem is solved as an ILP or LP" << endl;
	cout << "  2. Mode (key: mode). Default is satisficing. Can be set to optimal to make the heuristic optimal." << endl;
	cout << "  3. TDG (key: tdg). Default allowUC (allow unconnected). Can be set to uc (full encoding, no unconnected parts) or none." << endl;
	cout << "  4. PG (key: pg). Default is none. Can be set to full or relaxed to enable full or relaxed planning graph." << endl;
	cout << "  5. And/Or Landmarks (key: andOrLM). Default is None." << endl;
	cout << "     Can be set to full or onlyTNi to include all landmarks or only those derivable from the initial task network" << endl;
	cout << "  6. External landmarks (key: externalLM). Default is no. With none can be set to use external landmarks." << endl;
	cout << "  7. LM-cut (key: lmclmc). Default is full. Can be set to none to not include LM-cut landmarks" << endl;
	cout << "  8. Netchange Constraints (key: netchange). Default is full. Can be set to none to not include net-change constraints" << endl;
}



int main(int argc, char *argv[]) {
	//speed_test();
	//return 42;
#ifndef NDEBUG
    cout
            << "You have compiled the search engine without setting the NDEBUG flag. This will make it slow and should only be done for debug."
            << endl;
#endif

	gengetopt_args_info args_info;
	if (cmdline_parser(argc, argv, &args_info) != 0) return 1;

	// set debug mode
	if (args_info.debug_given) setDebugMode(true);

	int seed = args_info.seed_arg; // has default value
	cout << "Random seed: " << seed << endl;
	srand(seed);
    
	long timeL = args_info.timelimit_arg;

	// get input files
	std::vector<std::string> inputFiles;
	for ( unsigned i = 0 ; i < args_info.inputs_num ; ++i )
    	inputFiles.push_back(args_info.inputs[i]);

	std::string inputFilename = "-";

	if (inputFiles.size() > 1){
		std::cerr << "You may specify at most one file as input: the SAS+ problem description" << std::endl;
		return 1;
	} else {
		if (inputFiles.size())
			inputFilename = inputFiles[0];
	}

	std::istream * inputStream;
	if (inputFilename == "-") {
		std::cout << "Reading input from standard input." << std::endl;
		inputStream = &std::cin;
	} else {
		std::cout << "Reading input from " << inputFilename << "." << std::endl;

		std::ifstream * fileInput = new std::ifstream(inputFilename);
		if (!fileInput->good())
		{
			std::cerr << "Unable to open input file " << inputFilename << ": " << strerror (errno) << std::endl;
			return 1;
		}

		inputStream = fileInput;
	}

	//

	bool useTaskHash = true;


    /* Read model */
    // todo: the correct value of maintainTaskRechability depends on the heuristic
    eMaintainTaskReachability reachability = mtrACTIONS;
	bool trackContainedTasks = useTaskHash;
    Model* htn = new Model(trackContainedTasks, reachability, true, true);
	htn->filename = inputFilename;
	htn->read(inputStream);
	assert(htn->isHtnModel);
    ofstream o;
    ComplexInference(
            htn,
            "domain.hddl",
            "task.hddl",
            false, o,
            true);
    htn->generateMethodRepresentation();
	searchNode* tnI = htn->prepareTNi(htn);
	if (inputFilename != "-") ((ifstream*) inputStream)->close();
	
    if(reachability != mtrNO) {
        htn->calcSCCs();
        htn->calcSCCGraph();

        // add reachability information to initial task network
        htn->updateReachability(tnI);
    }

//    Heuristic *hLMC = new hhLMCount(htn, 0, tnI, lmfFD);
    eEstimate estimate = estDISTANCE;
    int hLength = 1; // enforce the number of heruistics to be used to 1
    aStar aStarType;
    int aStarWeight = 2;
    Heuristic **heuristics = new Heuristic*[hLength];
    if (args_info.config_arg == 1) {
        heuristics[hLength - 1] = new hhRC2<hsAddFF>(
                htn, hLength - 1,
                estimate, true);
        ((hhRC2<hsAddFF>*)heuristics[hLength - 1])->sasH->heuristic = sasAdd;
        aStarType = gValNone;
        bool suboptimalSearch = args_info.suboptimal_flag;
        cout << "Search config:" << endl;
        cout << "- Time limit: " << timeL << " seconds" << endl;
        cout << " - type: ";
        switch (aStarType){
            case gValNone: cout << "greedy"; break;
            case gValActionCosts: cout << "cost optimal"; break;
            case gValPathCosts: cout << "path cost"; break;
            case gValActionPathCosts: cout << "action cost + decomposition cost"; break;
        }
        cout << endl;
        cout << " - weight: " << aStarWeight << endl;
        cout << " - suboptimal: " << (suboptimalSearch?"true":"false") << endl;

        bool noVisitedList = args_info.noVisitedList_flag;
        bool allowGIcheck = args_info.noGIcheck_flag;
        bool taskHash = args_info.noTaskHash_flag;
        bool taskSequenceHash = args_info.noTaskSequenceHash_flag;
        bool topologicalOrdering = args_info.noTopologicalOrdering_flag;
        bool orderPairsHash = args_info.noOrderPairs_flag;
        bool layerHash = args_info.noLayers_flag;
        bool allowParalleSequencesMode = args_info.noParallelSequences_flag;

        VisitedList visi(
                htn, noVisitedList,
                suboptimalSearch,
                taskHash, taskSequenceHash,
                topologicalOrdering,
                orderPairsHash, layerHash,
                allowGIcheck,
                allowParalleSequencesMode);
        PriorityQueueSearch search;
        OneQueueWAStarFringe fringe(aStarType, aStarWeight, hLength);
        cout << "Start the Inner Planner" << endl;
        timeval tp;
        gettimeofday(&tp, NULL);
        long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
        long currentT;
        searchNode* sol = search.search(
                htn, tnI, timeL,
                suboptimalSearch,
                true,
                false,
                heuristics,
                hLength, visi, fringe);
        gettimeofday(&tp, NULL);
        currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
        long newTimeLimit = timeL - (currentT - startT);
        if (sol != nullptr) {
            auto [plan, length] = extractSolutionFromSearchNode(htn, sol);
            ofstream ofile;
            ofile.open(args_info.output_arg);
            ofile << plan << endl;
            ofile.close();
            return 0;
        } else {
            cout << "Inner planner failed to find a solution" << endl;
            cout << "Read the original model again" << endl;
            inputStream = new std::ifstream(inputFilename);
            htn->read(inputStream);
            tnI = htn->prepareTNi(htn);
            if (reachability != mtrNO) {
                htn->calcSCCs();
                htn->calcSCCGraph();

                // add reachability information to initial task network
                htn->updateReachability(tnI);
            }
            // TODO: check this!
            aStarType = gValActionPathCosts;
            aStarWeight = 2;
            cout << "Search config:" << endl;
            cout << "Time limit: " << newTimeLimit << " seconds" << endl;
            cout << " - type: ";
            switch (aStarType) {
                case gValNone:
                    cout << "greedy";
                    break;
                case gValActionCosts:
                    cout << "cost optimal";
                    break;
                case gValPathCosts:
                    cout << "path cost";
                    break;
                case gValActionPathCosts:
                    cout << "action cost + decomposition cost";
                    break;
            }
            cout << endl;
            cout << " - weight: " << aStarWeight << endl;
            cout << " - suboptimal: " << (suboptimalSearch ? "true" : "false") << endl;
            heuristics[hLength - 1] = new hhRC2<hsAddFF>(htn, hLength - 1,
                                                         estimate, true);
            ((hhRC2<hsAddFF> *) heuristics[hLength - 1])->sasH->heuristic = sasFF;
            VisitedList nextVisi(
                    htn, noVisitedList,
                    suboptimalSearch,
                    taskHash, taskSequenceHash,
                    topologicalOrdering,
                    orderPairsHash, layerHash,
                    allowGIcheck,
                    allowParalleSequencesMode);
            PriorityQueueSearch nextSearch;
            OneQueueWAStarFringe nextFringe(aStarType, aStarWeight, hLength);

            searchNode *sol = nextSearch.search(
                    htn, tnI,
                    newTimeLimit,
                    suboptimalSearch,
                    false,
                    false,
                    heuristics,
                    hLength, nextVisi,
                    nextFringe);
            if (sol != nullptr) {
                auto [plan, length] = extractSolutionFromSearchNode(htn, sol);
                ofstream ofile;
                ofile.open(args_info.output_arg);
                ofile << plan << endl;
                ofile.close();
                return 0;
            }
        }
    } else if (args_info.config_arg == 2) {
        heuristics[hLength - 1] = new hhRC2<hsAddFF>(
                htn, hLength - 1,
                estimate, true);
        ((hhRC2<hsAddFF>*)heuristics[hLength - 1])->sasH->heuristic = sasFF;
        aStarType = gValNone;
        bool suboptimalSearch = args_info.suboptimal_flag;
        cout << "Search config:" << endl;
        cout << "Time limit: " << timeL << " seconds" << endl;
        cout << " - type: ";
        switch (aStarType){
            case gValNone: cout << "greedy"; break;
            case gValActionCosts: cout << "cost optimal"; break;
            case gValPathCosts: cout << "path cost"; break;
            case gValActionPathCosts: cout << "action cost + decomposition cost"; break;
        }
        cout << endl;
        cout << " - weight: " << aStarWeight << endl;
        cout << " - suboptimal: " << (suboptimalSearch?"true":"false") << endl;

        bool noVisitedList = args_info.noVisitedList_flag;
        bool allowGIcheck = args_info.noGIcheck_flag;
        bool taskHash = args_info.noTaskHash_flag;
        bool taskSequenceHash = args_info.noTaskSequenceHash_flag;
        bool topologicalOrdering = args_info.noTopologicalOrdering_flag;
        bool orderPairsHash = args_info.noOrderPairs_flag;
        bool layerHash = args_info.noLayers_flag;
        bool allowParalleSequencesMode = args_info.noParallelSequences_flag;

        VisitedList visi(
                htn, noVisitedList,
                suboptimalSearch,
                taskHash, taskSequenceHash,
                topologicalOrdering,
                orderPairsHash, layerHash,
                allowGIcheck,
                allowParalleSequencesMode);
        PriorityQueueSearch search;
        OneQueueWAStarFringe fringe(aStarType, aStarWeight, hLength);
        cout << "Start the Inner Planner" << endl;
        timeval tp;
        gettimeofday(&tp, NULL);
        long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
        long currentT;
        searchNode* sol = search.search(
                htn, tnI, timeL,
                suboptimalSearch,
                true,
                false,
                heuristics,
                hLength, visi, fringe);
        gettimeofday(&tp, NULL);
        currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
        long newTimeLimit = timeL - ((currentT - startT)/1000);
        if (sol != nullptr) {
            auto [plan, length] = extractSolutionFromSearchNode(htn, sol);
            ofstream ofile;
            ofile.open(args_info.output_arg);
            ofile << plan << endl;
            ofile.close();
            return 0;
        } else {
            cout << "Inner planner failed to find a solution" << endl;
            cout << "Read the original model again" << endl;
            inputStream = new std::ifstream(inputFilename);
            htn->read(inputStream);
            tnI = htn->prepareTNi(htn);
            if (reachability != mtrNO) {
                htn->calcSCCs();
                htn->calcSCCGraph();

                // add reachability information to initial task network
                htn->updateReachability(tnI);
            }
            // TODO: check this!
            aStarType = gValActionPathCosts;
            aStarWeight = 2;
            cout << "Search config:" << endl;
            cout << "Time limit: " << newTimeLimit << " seconds" << endl;
            cout << " - type: ";
            switch (aStarType) {
                case gValNone:
                    cout << "greedy";
                    break;
                case gValActionCosts:
                    cout << "cost optimal";
                    break;
                case gValPathCosts:
                    cout << "path cost";
                    break;
                case gValActionPathCosts:
                    cout << "action cost + decomposition cost";
                    break;
            }
            cout << endl;
            cout << " - weight: " << aStarWeight << endl;
            cout << " - suboptimal: " << (suboptimalSearch ? "true" : "false") << endl;
            heuristics[hLength - 1] = new hhRC2<hsAddFF>(htn, hLength - 1,
                                                         estimate, true);
            ((hhRC2<hsAddFF> *) heuristics[hLength - 1])->sasH->heuristic = sasFF;
            VisitedList nextVisi(
                    htn, noVisitedList,
                    suboptimalSearch,
                    taskHash, taskSequenceHash,
                    topologicalOrdering,
                    orderPairsHash, layerHash,
                    allowGIcheck,
                    allowParalleSequencesMode);
            PriorityQueueSearch nextSearch;
            OneQueueWAStarFringe nextFringe(aStarType, aStarWeight, hLength);

            searchNode *sol = nextSearch.search(
                    htn, tnI,
                    newTimeLimit,
                    suboptimalSearch,
                    false,
                    false,
                    heuristics,
                    hLength, nextVisi,
                    nextFringe);
            if (sol != nullptr) {
                auto [plan, length] = extractSolutionFromSearchNode(htn, sol);
                ofstream ofile;
                ofile.open(args_info.output_arg);
                ofile << plan << endl;
                ofile.close();
                return 0;
            }
        }
    } else if (args_info.config_arg == 3) {
        heuristics[hLength - 1] = new hhRC2<hsAddFF>(
                htn, hLength - 1,
                estimate, true);
        ((hhRC2<hsAddFF>*)heuristics[hLength - 1])->sasH->heuristic = sasAdd;
        aStarWeight = 2;
        aStarType = gValNone;
        cout << "Start the Inner Planner" << endl;
        timeval tp;
        long newTimeLimit;
        bool suboptimalSearch = args_info.suboptimal_flag;
        cout << "First round search" << endl;
        cout << "Search config:" << endl;
        cout << " - Time limit: " << timeL << " seconds" << endl;
        cout << " - type: ";
        switch (aStarType){
            case gValNone: cout << "greedy"; break;
            case gValActionCosts: cout << "cost optimal"; break;
            case gValPathCosts: cout << "path cost"; break;
            case gValActionPathCosts: cout << "action cost + decomposition cost"; break;
        }
        cout << endl;
        cout << " - weight: " << aStarWeight << endl;
        cout << " - suboptimal: " << (suboptimalSearch?"true":"false") << endl;

        bool noVisitedList = args_info.noVisitedList_flag;
        bool allowGIcheck = args_info.noGIcheck_flag;
        bool taskHash = args_info.noTaskHash_flag;
        bool taskSequenceHash = args_info.noTaskSequenceHash_flag;
        bool topologicalOrdering = args_info.noTopologicalOrdering_flag;
        bool orderPairsHash = args_info.noOrderPairs_flag;
        bool layerHash = args_info.noLayers_flag;
        bool allowParalleSequencesMode = args_info.noParallelSequences_flag;
        VisitedList visi(
                htn, noVisitedList,
                suboptimalSearch,
                taskHash, taskSequenceHash,
                topologicalOrdering,
                orderPairsHash, layerHash,
                allowGIcheck,
                allowParalleSequencesMode);
        PriorityQueueSearch search;
        OneQueueWAStarFringe fringe(aStarType, aStarWeight, hLength);
        gettimeofday(&tp, NULL);
        long currentT;
        long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
        searchNode *sol = search.search(
                htn, tnI, timeL,
                suboptimalSearch,
                true,
                false,
                heuristics,
                hLength, visi, fringe);
        gettimeofday(&tp, NULL);
        currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
        newTimeLimit = timeL - ((currentT - startT)/1000);
        if (sol != nullptr) {
            cout << "Solution found in the first round" << endl;
            cout << "Second Round Search" << endl;
            aStarType = gValActionCosts;
            cout << "Search config:" << endl;
            cout << " - Time limit: " << to_string(newTimeLimit) << " seconds" << endl;
            cout << " - type: ";
            switch (aStarType) {
                case gValNone:
                    cout << "greedy";
                    break;
                case gValActionCosts:
                    cout << "cost optimal";
                    break;
                case gValPathCosts:
                    cout << "path cost";
                    break;
                case gValActionPathCosts:
                    cout << "action cost + decomposition cost";
                    break;
            }
            cout << endl;
            cout << " - weight: " << aStarWeight << endl;
            cout << " - suboptimal: " << (suboptimalSearch ? "true" : "false") << endl;
            tnI = htn->prepareTNi(htn);
            heuristics[hLength - 1] = new hhRC2<hsAddFF>(htn, hLength - 1, estimate, true);
            ((hhRC2<hsAddFF> *) heuristics[hLength - 1])->sasH->heuristic = sasFF;
            VisitedList nextVisi(
                    htn, noVisitedList,
                    suboptimalSearch,
                    taskHash, taskSequenceHash,
                    topologicalOrdering,
                    orderPairsHash, layerHash,
                    allowGIcheck,
                    allowParalleSequencesMode);
            PriorityQueueSearch nextSearch;
            OneQueueWAStarFringe nextFringe(aStarType, aStarWeight, hLength);
            gettimeofday(&tp, NULL);
            startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
            searchNode *nextSol = nextSearch.search(
                    htn, tnI, newTimeLimit,
                    suboptimalSearch,
                    true, false, heuristics,
                    hLength, nextVisi,
                    nextFringe, sol->actionCosts);
            gettimeofday(&tp, NULL);
            currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
            newTimeLimit = newTimeLimit - ((currentT - startT)/1000);
            if (nextSol != nullptr && nextSol->actionCosts <= sol->actionCosts) {
                sol = nextSol;
                cout << "Found better solution in the second round search" << endl;
            }
            if (newTimeLimit > 0) {
                cout << "Third Round Search" << endl;
                aStarType = gValActionCosts;
                aStarWeight = 1;
                tnI = htn->prepareTNi(htn);
                estimate = estCOSTS;
                heuristics[hLength - 1] = new hhRC2<hsLmCut>(htn, hLength - 1, estimate, true);
                cout << "Search config:" << endl;
                cout << " - Time limit: " << to_string(newTimeLimit) << " seconds" << endl;
                cout << " - type: ";
                switch (aStarType) {
                    case gValNone:
                        cout << "greedy";
                        break;
                    case gValActionCosts:
                        cout << "cost optimal";
                        break;
                    case gValPathCosts:
                        cout << "path cost";
                        break;
                    case gValActionPathCosts:
                        cout << "action cost + decomposition cost";
                        break;
                }
                cout << endl;
                cout << " - weight: " << aStarWeight << endl;
                cout << " - suboptimal: " << (suboptimalSearch ? "true" : "false") << endl;
                VisitedList thirdVisi(
                        htn, noVisitedList,
                        suboptimalSearch,
                        taskHash, taskSequenceHash,
                        topologicalOrdering,
                        orderPairsHash, layerHash,
                        allowGIcheck,
                        allowParalleSequencesMode);
                PriorityQueueSearch thirdSearch;
                OneQueueWAStarFringe thirdFringe(aStarType, aStarWeight, hLength);
                searchNode *thirdSol = thirdSearch.search(
                        htn, tnI, newTimeLimit,
                        suboptimalSearch,
                        true, false, heuristics,
                        hLength, thirdVisi,
                        thirdFringe);
                if (thirdSol != nullptr) {
                    cout << "Fount an optimal solution" << endl;
                    sol = thirdSol;
                }
            }
            auto [plan, length] = extractSolutionFromSearchNode(htn, sol);
            ofstream ofile;
            ofile.open(args_info.output_arg);
            ofile << plan << endl;
            ofile.close();
            return 0;
        } else {
            cout << "Inner planner failed to find a solution" << endl;
            cout << "Read the original model again" << endl;
            inputStream = new std::ifstream(inputFilename);
            htn->read(inputStream);
            tnI = htn->prepareTNi(htn);
            if (reachability != mtrNO) {
                htn->calcSCCs();
                htn->calcSCCGraph();

                // add reachability information to initial task network
                htn->updateReachability(tnI);
            }
            // TODO: check this!
            aStarType = gValActionPathCosts;
            aStarWeight = 2;
            cout << "Search config:" << endl;
            cout << " - type: ";
            switch (aStarType) {
                case gValNone:
                    cout << "greedy";
                    break;
                case gValActionCosts:
                    cout << "cost optimal";
                    break;
                case gValPathCosts:
                    cout << "path cost";
                    break;
                case gValActionPathCosts:
                    cout << "action cost + decomposition cost";
                    break;
            }
            cout << endl;
            cout << " - weight: " << aStarWeight << endl;
            cout << " - suboptimal: " << (suboptimalSearch ? "true" : "false") << endl;
            heuristics[hLength - 1] = new hhRC2<hsAddFF>(htn, hLength - 1,
                                                         estimate, true);
            ((hhRC2<hsAddFF> *) heuristics[hLength - 1])->sasH->heuristic = sasFF;
            VisitedList nextVisi(
                    htn, noVisitedList,
                    suboptimalSearch,
                    taskHash, taskSequenceHash,
                    topologicalOrdering,
                    orderPairsHash, layerHash,
                    allowGIcheck,
                    allowParalleSequencesMode);
            PriorityQueueSearch nextSearch;
            OneQueueWAStarFringe nextFringe(aStarType, aStarWeight, hLength);

            searchNode *sol = nextSearch.search(
                    htn, tnI,
                    newTimeLimit,
                    suboptimalSearch,
                    false,
                    false,
                    heuristics,
                    hLength, nextVisi,
                    nextFringe);
            if (sol != nullptr) {
                auto [plan, length] = extractSolutionFromSearchNode(htn, sol);
                ofstream ofile;
                ofile.open(args_info.output_arg);
                ofile << plan << endl;
                ofile.close();
                return 0;
            }
        }
    } else {std::cerr << "Illegal configuration"; exit(-1);}
}








/*
    long initO, initN;
    long genO, genN;
    long initEl = 0;
    long genEl;
    long start, end;
    long tlmEl;
    long flmEl = 0;
    long mlmEl = 0;
    long tlmO, flmO, mlmO, tlmN, flmN, mlmN;

    timeval tp;
    gettimeofday(&tp, NULL);
    start = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    LmCausal* lmc = new LmCausal(htn);
    lmc->prettyprintAndOrGraph();
    gettimeofday(&tp, NULL);
    end = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    initN = end - start;

    gettimeofday(&tp, NULL);
    start = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    LMsInAndOrGraphs* ao = new LMsInAndOrGraphs(htn);
    gettimeofday(&tp, NULL);
    end = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    initO = end - start;

    gettimeofday(&tp, NULL);
    start = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	lmc->calcLMs(tnI);
    gettimeofday(&tp, NULL);
    end = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    genN = end - start;

    tlmN = landmark::coutLM(lmc->getLMs(), task, lmc->numLMs);
    flmN = landmark::coutLM(lmc->getLMs(), fact, lmc->numLMs);
    mlmN = landmark::coutLM(lmc->getLMs(), METHOD, lmc->numLMs);

    gettimeofday(&tp, NULL);
    start = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    ao->generateAndOrLMs(tnI);
    gettimeofday(&tp, NULL);
    end = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    genO = end - start;

    tlmO = landmark::coutLM(ao->getLMs(), task, ao->getNumLMs());
    flmO = landmark::coutLM(ao->getLMs(), fact, ao->getNumLMs());
    mlmO = landmark::coutLM(ao->getLMs(), METHOD, ao->getNumLMs());

    if(lmc->numLMs != ao->getNumLMs()) {
        cout << "AAAAAAAAAAAAAAAAAAAAHHH " << ao->getNumLMs() << " - " << lmc->numLMs << endl;
        for(int i = 0; i < ao->getNumLMs(); i ++) {
            ao->getLMs()[i]->printLM();
        }
        cout << "----------------------" << endl;
        for(int i = 0; i < lmc->numLMs; i++) {
            lmc->landmarks[i]->printLM();
        }
    }

    cout << "TIME:" << endl;
    cout << "Init       : " << initO << " " << initN << " delta " << (initN - initO) << endl;
    cout << "Generation : " << genO << " " << genN << " delta " << (genN - genO) << endl;
    cout << "Total      : " << (initO + genO) << " " << (initN + genN) << " delta " << ((initN + genN) - (initO + genO)) << endl;

    gettimeofday(&tp, NULL);
    start = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    ao->generateLocalLMs(htn, tnI);
    gettimeofday(&tp, NULL);
    end = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    genEl = end - start;

    tlmEl = landmark::coutLM(ao->getLMs(), task, ao->getNumLMs());
    flmEl = landmark::coutLM(ao->getLMs(), fact, ao->getNumLMs());
    mlmEl = landmark::coutLM(ao->getLMs(), METHOD, ao->getNumLMs());

    cout << "LMINFO:[" << s << ";";
    cout << initEl << ";" << genEl << ";" << (initEl + genEl) << ";";
    cout << initO << ";" << genO << ";" << (initO + genO) << ";";
    cout << initN << ";" << genN << ";" << (initN + genN) << ";";
    cout << tlmEl << ";" << flmEl << ";" << mlmEl << ";";
    cout << tlmO << ";" << flmO << ";" << mlmO << ";";
    cout << tlmN << ";" << flmN << ";" << mlmN << ";";
    cout << "]" << endl;

	//lmc->prettyprintAndOrGraph();
    for(int i = 0; i < htn->numTasks; i++)
        cout << i << " " << htn->taskNames[i] << endl;
    for(int i = 0; i < htn->numStateBits; i++)
        cout << i << " " << htn->factStrs[i] << endl;

    cout << "AND/OR landmarks" << endl;
    for(int i = 0; i < lmc->numLMs; i++) {
        lmc->getLMs()[i]->printLM();
    }
    cout << "Local landmarks" << endl;
    for(int i = 0; i < ao->getNumLMs(); i++) {
       ao->getLMs()[i]->printLM();
    }

    cout << "PRINT" << endl;
    lmc->prettyPrintLMs();

	exit(17);*/

