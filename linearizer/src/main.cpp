#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <map>
#include <vector>
#include <unordered_set>

#include "cwa.hpp"
#include "domain.hpp"
#include "hddl.hpp"
#include "hddlWriter.hpp"
#include "hpdlWriter.hpp"
#include "htn2stripsWriter.hpp"
#include "output.hpp"
#include "parametersplitting.hpp"
#include "parsetree.hpp"
#include "plan.hpp"
#include "shopWriter.hpp"
#include "sortexpansion.hpp"
#include "typeof.hpp"
#include "util.hpp"
#include "verify.hpp"
#include "verification_encoding.hpp"
#include "properties.hpp"

#include "directed_graph.h"
// #include "../../pandaPIengine/src/intDataStructures/IntPairHeap.h"
// #include "../../pandaPIengine/src/Model.h"
// #include "../../code/src/main_small.cpp"
// #include "../../pandaPIengine/src/Model.h"
// #include "../../PandaPOPrecsEffs-preEff/src/Model.h"
#include "cmdline.h"

using namespace std;

// declare parser function manually
void run_parser_on_file(FILE *f, char *filename);

// parsed domain data structures
bool has_typeof_predicate = false;
vector<sort_definition> sort_definitions;
vector<predicate_definition> predicate_definitions;
vector<parsed_task> parsed_primitive;
vector<parsed_task> parsed_abstract;
map<string, vector<parsed_method>> parsed_methods;
vector<pair<predicate_definition, string>> parsed_functions;
string metric_target = dummy_function_type;

map<string, set<string>> sorts; // type,  set of objects of that type, e.g. (waypoint : {waypoint1, waypoint2},  rover: {rover1} )
vector<method> methods;
vector<task> primitive_tasks;
vector<task> abstract_tasks;

map<string, task> task_name_map;

//////////////////////////////////////  START OF STUFF THAT ying ADDED /////////////////////////////////////////////
// printf("\n\nabstract tasks\n");
// for (auto task : abstract_tasks)
// {
// 	printf("%s \n", task.name.c_str());
// 	// string, task
// }
// printf("\n\nmethod names\n");
// for (auto method : methods)
// {
// 	printf("%s  %s\n", method.at.c_str(), method.name.c_str()); // name of the task it decomposes?
// 	// vars
// 	for (auto var : method.vars)
// 	{
// 		printf("(%s %s) ", var.first.c_str(), var.second.c_str()); // (?to waypoint) (?from waypoint)
// 	}
// 	printf("\n");
// 	// plan steps
// 	for (auto step : method.ps)
// 	{
// 		printf("%s ,", step.task.c_str());
// 	}
// 	printf("\n");
// 	// constranst
// 	for (auto cons : method.constraints)
// 	{
// 		printf("%s ,", cons.predicate.c_str());
// 	}
// 	printf("\n");
// 	printf("\n");
//
// 	// vector<plan_step> ps;
// 	//  vector<literal> constraints;
// }


struct pair_hash {
    inline std::size_t operator()(const std::pair<string,string> & v) const {
		return std::hash<std::string>()(v.first + v.second);
    }
};

struct param {
	string type;            //  :parameters (?s - store ?rover - rover)
	string abstract_name;   //  :parameters (?s - store ?rover - rover)
	string instance_name;   //                    store_1,       rover_3

	param(string t, string abstract, string instance) : 
	type(t), abstract_name(abstract), instance_name(instance) {}
};


void add_literal(unordered_set<string> &vec, literal l, vector<param> &instantiated_parameters)
{
	string temp_predicate = l.predicate;
	for (auto param : instantiated_parameters) {
		temp_predicate = temp_predicate + string(param.instance_name);
	}
	vec.insert(temp_predicate);
}

// assumption: equality between different names is possible
// assumption: equality between same name is guaranteed
param get_param_name(pair<string,string> arg, map<string, int> &var_for_method) {
	string type = arg.second;
	string abstract_name = arg.first;
	string instance_name = "__" + type + "_" + to_string(var_for_method[type]);  	 // e.g. _waypoint_1
	param p = param(type, abstract_name, instance_name);
	var_for_method[type]++;
	return p;
}
                                
vector<param> match_arg(vector<pair<string,string>> vars, vector<string> args, vector<param> instantiated_parameters, map<string, int> var_for_method) {
	vector<param> new_instantiated_parameters;

	for (unsigned long i=0; i< args.size(); i++) {
		for (unsigned long j=0; j<vars.size(); j++) {
			string plan_step_abstract_param = args[i] ; 		
			pair<string, string> function_call_abstract_param = vars[j]; // .at(i);
			// we HAVE to match them like this,  method args != task it decomposes vars  
			if (plan_step_abstract_param.compare(function_call_abstract_param.first) == 0) {
				// printf("want to match %s=%s to  %s)\n", plan_step_abstract_param.c_str(), function_call_abstract_param.first.c_str(), function_call_abstract_param.second.c_str());
				bool found = false; 
				for (param p : instantiated_parameters) {
					if ((p.abstract_name).compare(plan_step_abstract_param) == 0) {
						found  = true;
						// printf("match arg (%s/%s -> %s) ", p.instance_name.c_str(), p.abstract_name.c_str(), function_call_abstract_param.first.c_str());
						new_instantiated_parameters.push_back( param(p.type, function_call_abstract_param.first, p.instance_name) );
					}				
				}
				// if no match for some variable, make your own
				if (!found)
				{
					param new_temp_var = get_param_name(function_call_abstract_param, var_for_method);	
					//printf("new arg (%s/%s -> %s) ", new_temp_var.instance_name.c_str(), new_temp_var.abstract_name.c_str(), function_call_abstract_param.first.c_str());
					new_instantiated_parameters.push_back( new_temp_var );		
				}
			}
		}
	}
	return new_instantiated_parameters;
}

bool in_array(string s, vector<string> array) {
	for (auto ss : array) {
		if (s.compare(ss) == 0) {
			return true;
		}
	}
	return false;
}

bool in_set(string s, unordered_set<string> array) {
	for (auto ss : array) {
		if (s.compare(ss) == 0) {
			return true;
		}
	}
	return false;
}

// method m
// instantiated_parameters .  each param is (type, name) e.g. (waypoint, temp_waypoint_1) (parameters of the method that produced this task)
// precs, adds, deletes = for one task, predicates (as string) 
// TODO : and dealing with the recursion
void process_primitive_task(task task, vector<param> instantiated_parameters,  map<string, int> var_for_method,  unordered_set<string> seen_before,
					unordered_set<string> &precs, unordered_set<string> &adds, unordered_set<string> &deletes)
{
	// printf("Process primitive task: %s* \n", task.name.c_str());

	for (auto p : task.prec)
	{	
		vector<param> new_instantiated_parameters = match_arg(task.vars, p.arguments, instantiated_parameters, var_for_method);
		add_literal(precs, p, new_instantiated_parameters);  //using instantiated parameters instead
	}

	for (auto eff : task.eff)
	{
		if (eff.positive)
		{
			vector<param> new_instantiated_parameters = match_arg(task.vars, eff.arguments, instantiated_parameters, var_for_method);
			add_literal(adds, eff, new_instantiated_parameters);
		}
		else
		{
			vector<param> new_instantiated_parameters = match_arg(task.vars, eff.arguments, instantiated_parameters, var_for_method);
			add_literal(deletes, eff, new_instantiated_parameters);
		}
	}

	// task.ceff.condition;
	// task.ceff.effect;
	// for (auto ceff : task.ceff)
	// {
	// 	for (auto cond : ceff.condition)
	// 	{
	// 		vector<param> new_instantiated_parameters = match_arg(task.vars, cond.arguments, instantiated_parameters, var_for_method);
	// 		add_literal(precs, cond, new_instantiated_parameters);
	// 	}
	// 	if (ceff.effect.positive)
	// 	{
	// 		vector<param> new_instantiated_parameters = match_arg(task.vars, ceff.effect.arguments, instantiated_parameters, var_for_method);
	// 		add_literal(adds, ceff.effect, instantiated_parameters);
	// 	}
	// 	else
	// 	{
	// 		add_literal(deletes, ceff.effect, instantiated_parameters);
	// 	}
	// }

	// task.constraints;
}

void process_method(method m, vector<param> instantiated_parameters,  map<string, int> var_for_method,  unordered_set<string> seen_before,
					unordered_set<string> &precs, unordered_set<string> &adds, unordered_set<string> &deletes);

// STEP 1) Given an abstract task, and all instantiated parameters that apply,
void process_abstract_task(task task, vector<param> instantiated_parameters,  map<string, int> var_for_method, unordered_set<string> seen_before,
					unordered_set<string> &precs, unordered_set<string> &adds, unordered_set<string> &deletes)
{
	if (in_set(task.name, seen_before)) {
		return;
	} else {
		seen_before.insert(task.name);
	}
	// STEP 2) ... match instantiates parameters to the children method's parameters
	// printf("Methods that apply to %s: \n", task.name.c_str());
	for (auto m : methods)
	{
		vector<param> new_instantiated_parameters; // new instantiated parameters, per method
		// all methods that apply to the abstract task
		if (m.at == task.name) {
			// m.vars, m.atargs, don't necessarily match
			new_instantiated_parameters = match_arg(m.vars, m.atargs, instantiated_parameters, var_for_method);
			process_method(m, new_instantiated_parameters, var_for_method, seen_before,  precs, adds, deletes);
		}
	}
	// printf("\n");
}

// STEP 1) // given instantiated parameters of the parent method ...
void process_method(method m, vector<param> instantiated_parameters,  map<string, int> var_for_method, unordered_set<string> seen_before,
					unordered_set<string> &precs, unordered_set<string> &adds, unordered_set<string> &deletes) {
	// STEP 2) ... match to the children task's parameters
	// printf("process_method %s: \n", m.name.c_str());
	for (auto step : m.ps)
	{
		string subtask_name = step.task; 

		// printf("  %s: " , subtask_name.c_str());				
		vector<param> new_instantiated_parameters; // new instantiated parameters, per subtask 
		for (task t : abstract_tasks) {
			if (subtask_name.compare(t.name) == 0) {
				if (!(in_set(t.name, seen_before))) {
					new_instantiated_parameters = match_arg(t.vars, step.args, instantiated_parameters, var_for_method);
					process_abstract_task(t, new_instantiated_parameters, var_for_method, seen_before, precs, adds, deletes);
					seen_before.insert(t.name);
				}
			}
		}
		for (task t : primitive_tasks) {
			if (subtask_name.compare(t.name) == 0) {
				if (!(in_set(t.name, seen_before))) {
					new_instantiated_parameters = match_arg(t.vars, step.args, instantiated_parameters, var_for_method);
					process_primitive_task(t, new_instantiated_parameters, var_for_method, seen_before, precs, adds, deletes);
					seen_before.insert(t.name);
				}
			}
		}
		// printf("\n");
	}	
	// printf("\n");
}

// method m
// instantiated_parameters .  each param is (type, name) e.g. (waypoint, temp_waypoint_1)
// tasks_precs, tasks_adds, tasks_deletes =  a vector of each tasks (adds, pre, del) for the subtasks of this method?
void collect_pre_eff_one_method(method m, vector<param> instantiated_parameters, 
						map<string, unordered_set<string>> &tasks_precs, map<string, unordered_set<string>> &tasks_adds, map<string, unordered_set<string>> &tasks_deletes)
{
	// what to call the next temp variable
	map<string, int> var_for_method; 
	// initialise the int marker, for new temp variables you want to instantiate
	for (auto s : sorts)
	{
		var_for_method[s.first] = 1;
	}

	// STEP 1)  give parameters to the method that is being decomposed
	// 	vector<pair<string,string>> vars;  // pair<string, string> =  var_type, var_name
	for (auto arg : m.vars) { 
		param p = get_param_name(arg, var_for_method);
		instantiated_parameters.push_back(p);
	}

	// STEP 2) given this method, deal with their children?
	for (auto step : m.ps)
	{
		string subtask_name = step.task; 
		unordered_set<string> precs;
		unordered_set<string> adds;
		unordered_set<string> deletes;
		unordered_set<string> seen_before;		
			
		for (auto task : primitive_tasks)
		{
			if (task.name.compare(subtask_name) == 0) {
				vector<param> new_instantiated_parameters = match_arg(task.vars, step.args, instantiated_parameters, var_for_method);						 
				process_primitive_task(task, new_instantiated_parameters, var_for_method, seen_before, precs, adds, deletes); 
			}
		}

		for (auto task : abstract_tasks)
		{
			if (task.name.compare(subtask_name) == 0) {
				vector<param> new_instantiated_parameters = match_arg(task.vars, step.args, instantiated_parameters, var_for_method);
				process_abstract_task(task, new_instantiated_parameters, var_for_method, seen_before, precs, adds, deletes); 
			}
		}
		tasks_precs[step.id] = precs;
		tasks_adds[step.id] = adds;
		tasks_deletes[step.id] = deletes;
	}

	// map<string, unordered_set<string>> :: iterator iter; 
	// for (iter = tasks_precs.begin(); iter != tasks_precs.end(); iter++)
	// {
		// string id = (*iter).first;
		// printf("Literals collecting for task, %s: \n ", id.c_str()); // %s:\n", task.name.c_str());
		
		// printf("precs: ");
		// for (auto p: tasks_precs[id])
		// {
		// 	printf("%s, ", p.c_str());
		// }
		
		// printf("\nadds: ");
		// for (auto p: tasks_adds[id])
		// {
		// 	printf("%s, ", p.c_str());
		// }
		
		// printf("\ndeletes: ");
		// for (auto p: tasks_deletes[id])
		// {
		// 	printf("%s, ", p.c_str());
		// }
		// printf("\n\n");		
	// } // END: RESULT OF THE LITERAL COLLECTING
}


void compare_preeff_vecs(map<string, unordered_set<string>> &head_predicates, map<string, unordered_set<string>> &tail_predicates, 
unordered_set< pair<string, string>, pair_hash> &tentative_orderings) {
	
	map<string, unordered_set<string>> :: iterator iter;
	for (iter = head_predicates.begin(); iter != head_predicates.end(); iter++)
	{
		string head_id = (*iter).first;
		unordered_set<string> head_precs = (*iter).second;
		// printf("head id %s\n", head_id.c_str());
		map<string, unordered_set<string>> :: iterator iter2; 
		for (iter2 = tail_predicates.begin(); iter2 != tail_predicates.end(); iter2++)
		{
			string tail_id = (*iter2).first;
			unordered_set<string> tail_adds = (*iter2).second;
			// printf("tail id %s\n", tail_id.c_str());

			for (string head_p : head_precs) {
				for (string tail_p : tail_adds) {
					if (head_p.compare(tail_p) == 0) {
						// printf("(%s -> %s), ", head_id.c_str(), tail_id.c_str());
						pair<string, string> new_ordering = pair(head_id, tail_id);						
						tentative_orderings.insert(new_ordering); 
					}
				}
			}
		}
	}
}

// contains the precs, adds, deletes,  for the subtasks for one method,  as strings like "at __rover_1 __waypoint_2"
unordered_set< pair<string, string>, pair_hash> find_more_orders_one_method(method m, map<string, unordered_set<string>> tasks_precs, map<string, unordered_set<string>> tasks_adds, map<string, unordered_set<string>> tasks_deletes)
{
	// convert to int array?  (m1_preeff + m2_preeff .. + mn_preeff)  to go through
	// checks = total_preeff * number of methods

	// for task1, preeff -> compare to all other tasks preeff
	// for task2, preff -> compare to all other tasks
	// checks = total_preeff * total_preeff

	// NOTE: m.ordering; //vector<pair<string,string>> ordering; 
	// ordering.push_back( pair(first, second) );

	// only draw orderings between *guaranteed* pre eff?

	unordered_set< pair<string, string>, pair_hash> tentative_orderings;

	// For each add effect a of c move all tasks with precondition a behind c
    //  and all tasks with a delete effect in front of it.  
	compare_preeff_vecs(tasks_adds, tasks_precs, tentative_orderings);  //  (add a) (prec a)  
	compare_preeff_vecs(tasks_deletes, tasks_adds, tentative_orderings);  //  (delete a) (add a)

    // For each delete effect d of c move all tasks with precondition d before c
    // and all tasks with an add effect behind it.
	compare_preeff_vecs(tasks_precs, tasks_deletes, tentative_orderings);  // (prec d) (delete d)  
	// compare_preeff_vecs(tasks_deletes, tasks_adds);  // (delete d) (add d)

	return tentative_orderings;
}

bool cycle_break_one_method(method m, unordered_set<pair<string, string>, pair_hash> old_orderings,
unordered_set<pair<string, string>, pair_hash> &linearised_orderings)
{
	std::set<edge> edges;
	bool needed_cycle_break = false;

	// from string task.id,  to int task_id
	map<string, int> mapping_subtasks;
	map<int, string> rev_mapping_subtasks;
	int i = 0;
	for (auto ps : m.ps) {
		mapping_subtasks[ps.id] = i;
		rev_mapping_subtasks[i] = ps.id;
		i++; 
	}

	// first priority : OG lines
	for (pair<string, string> p : m.ordering) {
		edge edge( mapping_subtasks[p.first], mapping_subtasks[p.second], 0);
		edges.insert( edge );
	}
	//  second priority to respect: output of find_orderings
	// (cannot overwrite previous edges (i.e. already assigned a higher priority) )
	for (pair<string, string> p : old_orderings) {
		edge edge(mapping_subtasks[p.first], mapping_subtasks[p.second], 1);
		edges.insert( edge );
	}

	// find and break cycles in proposed orderings (without affecting required orderings)
	int V = m.ps.size();
	std::set<edge> new_orderings = break_cycle(edges, V);

	if (new_orderings != edges)
	{
		needed_cycle_break = true;
	}

	// make new orderings totally ordered
	Graph g2(m.ps.size()); // (*m).numSubTasks[method]
	for (auto e : new_orderings)
		g2.addEdge(e.start, e.end, 0);
	stack<int> Stack = g2.topologicalSort();

	// return topological ordering in pairs format
	while (Stack.empty() == false)
	{
		int first = Stack.top(); 
		Stack.pop();
		if (!Stack.empty()) {
			int second = Stack.top(); 
			linearised_orderings.insert( pair(rev_mapping_subtasks[first], rev_mapping_subtasks[second])  );
			first = second;
		}
	}
	return needed_cycle_break;
}


void alter_one_method(method m, unordered_set<pair<string, string>, pair_hash> linearised_orderings) {
	for (unsigned long i=0; i<methods.size(); i++) {
		if ((methods[i].name).compare(m.name) == 0) {
			methods[i].ordering.clear(); 
			for (pair<string, string> p : linearised_orderings) {
				methods[i].ordering.push_back(p);
			}
		}
	} 
}


void linearize_one_method(method m)
{ 
	map<string, unordered_set<string>> tasks_precs;
	map<string, unordered_set<string>> tasks_adds;
	map<string, unordered_set<string>> tasks_deletes;
	vector<param> instantiated_parameters;
	
	// TODO: RUNNING collect_pre_eff_one_method IS CAUSING 
	// terminate called after throwing an instance of 'std::logic_error'
	//   what():  basic_string::_M_construct null not valid
	// Aborted (core dumped)
	// some string is not being instantiated correctly!
	collect_pre_eff_one_method(m, instantiated_parameters, tasks_precs, tasks_adds, tasks_deletes);

	unordered_set< pair<string, string>, pair_hash> tentative_orderings = find_more_orders_one_method(m, tasks_precs, tasks_adds, tasks_deletes);
	unordered_set<pair<string, string>, pair_hash> linearised_orderings;
	cycle_break_one_method(m, tentative_orderings, linearised_orderings);
	alter_one_method(m, linearised_orderings);

	
	// printf("Original\n");
	// for (auto pair: m.ordering) {
	// 	printf("(%s -> %s)", pair.first.c_str(), pair.second.c_str());
	// }
	// printf("\nLinearised\n");
	// for (auto pair: linearised_orderings) {
	// 	printf("(%s -> %s)", pair.first.c_str(), pair.second.c_str());
	// } 
	// printf("\n");
}

void linearize(bool removeMethodPreconditions, int doutfile, int poutfile)
{
	// the top method ( methods[0];) does NOT have abstract ?from ?to,  but actual waypoint2 waypoint3 objects	
	for (auto m : methods)
	{ 
		linearize_one_method(m);
	}
	printf("Finished linearizing all methods, writing to output.\n");
}
//////////////////////////////// END OF STUFF THAT I ADDED ////////////////////////////////////

int main(int argc, char **argv)
{
	cin.sync_with_stdio(false);
	cout.sync_with_stdio(false);
	int dfile = -1;
	int pfile = -1;
	int doutfile = -1;
	int poutfile = -1;
	bool splitParameters = true;
	bool compileConditionalEffects = true;
	bool linearConditionalEffectExpansion = false;
	bool encodeDisjunctivePreconditionsInMethods = false;
	bool compileGoalIntoAction = false;

	bool doVerifyEncoding = false;
	string verificationFile = "";

	bool shopOutput = false;
	bool hpdlOutput = false;
	bool htn2stripsOutput = false;
	bool pureHddlOutput = false;
	bool hddlOutput = false;
	bool internalHDDLOutput = false;
	bool dontWriteConstantsIntoDomain = false;
	bool lenientVerify = false;
	bool verboseOutput = false;
	bool verifyPlan = false;
	bool useOrderInPlanVerification = true;
	bool convertPlan = false;
	bool showProperties = false;
	bool removeMethodPreconditions = false;
	int verbosity = 0;

	gengetopt_args_info args_info;
	if (cmdline_parser(argc, argv, &args_info) != 0)
		return 1;

	// set debug mode
	if (args_info.debug_given)
	{
		verboseOutput = true;
		verbosity = args_info.debug_arg;
	}
	if (args_info.no_colour_given)
		no_colors_in_output = true;
	if (args_info.no_split_parameters_given)
		splitParameters = false;
	if (args_info.keep_conditional_effects_given)
		compileConditionalEffects = false;
	if (args_info.linear_conditional_effect_given)
	{
		compileConditionalEffects = false;
		linearConditionalEffectExpansion = true;
	}
	if (args_info.encode_disjunctive_preconditions_in_htn_given)
		encodeDisjunctivePreconditionsInMethods = true;
	if (args_info.goal_action_given)
		compileGoalIntoAction = true;
	if (args_info.remove_method_preconditions_given)
		removeMethodPreconditions = true;

	if (args_info.shop_given)
		shopOutput = true;
	if (args_info.shop1_given)
		shopOutput = shop_1_compatability_mode = true;
	if (args_info.hpdl_given)
		hpdlOutput = true;
	if (args_info.hppdl_given)
		htn2stripsOutput = true;
	if (args_info.hddl_given)
		pureHddlOutput = true;
	if (args_info.processed_hddl_given)
		hddlOutput = true;
	if (args_info.internal_hddl_given)
		hddlOutput = internalHDDLOutput = true;
	if (args_info.no_domain_constants_given)
		dontWriteConstantsIntoDomain = true;

	if (args_info.verify_given)
	{
		verifyPlan = true;
		verbosity = args_info.verify_arg;
	}
	if (args_info.vverify_given)
	{
		verifyPlan = true;
		verbosity = 1;
	}
	if (args_info.vvverify_given)
	{
		verifyPlan = true;
		verbosity = 2;
	}
	if (args_info.lenient_given)
		verifyPlan = lenientVerify = true;
	if (args_info.verify_no_order_given)
	{
		verifyPlan = true;
		useOrderInPlanVerification = false;
	}

	if (args_info.verification_encoding_given)
	{
		doVerifyEncoding = true;
		verificationFile = args_info.verification_encoding_arg;
	}

	if (args_info.panda_converter_given)
		convertPlan = true;
	if (args_info.properties_given)
		showProperties = true;

	cout << "pandaPIparser is configured as follows" << endl;
	cout << "  Colors in output: " << boolalpha << !no_colors_in_output << endl;
	if (showProperties)
	{
		cout << "  Mode: show instance properties" << endl;
	}
	else if (convertPlan)
	{
		cout << "  Mode: convert pandaPI plan" << endl;
	}
	else if (verifyPlan)
	{
		cout << "  Mode: plan verification" << endl;
		cout << "  Verbosity: " << verbosity << endl;
		cout << "  Lenient mode: " << boolalpha << lenientVerify << endl;
		cout << "  Ignore given order: " << !useOrderInPlanVerification << endl;
	}
	else
	{
		cout << "  Mode: parsing mode" << endl;
		cout << "  Parameter splitting: " << boolalpha << splitParameters << endl;
		cout << "  Conditional effects: ";
		if (compileConditionalEffects)
		{
			if (linearConditionalEffectExpansion)
				cout << "linear encoding";
			else
				cout << "exponential encoding";
		}
		else
			cout << "keep";
		cout << endl;
		cout << "  Disjunctive preconditions as HTN: " << boolalpha << encodeDisjunctivePreconditionsInMethods << endl;
		cout << "  Replace goal with action: " << boolalpha << compileGoalIntoAction << endl;

		cout << "  Output: ";
		if (shopOutput)
			cout << "SHOP2";
		else if (shop_1_compatability_mode)
			cout << "SHOP1";
		else if (hpdlOutput)
			cout << "HPDL";
		else if (htn2stripsOutput)
			cout << "HPPDL";
		else if (pureHddlOutput)
			cout << "HDDL (no transformations)";
		else if (hddlOutput && internalHDDLOutput)
			cout << "HDDL (internal)";
		else if (hddlOutput && !internalHDDLOutput)
			cout << "HDDL (with transformations)";
		else
			cout << "pandaPI format";
		cout << endl;
	}

	vector<string> inputFiles;
	for (unsigned i = 0; i < args_info.inputs_num; i++)
		inputFiles.push_back(args_info.inputs[i]);

	if (inputFiles.size() > 0)
		dfile = 0;
	if (inputFiles.size() > 1)
		pfile = 1;
	if (inputFiles.size() > 2)
		doutfile = 2;
	if (inputFiles.size() > 3)
		poutfile = 3;

	if (dfile == -1)
	{
		if (convertPlan)
			cout << "You need to provide a plan as input." << endl;
		else
			cout << "You need to provide a domain and problem file as input." << endl;
		return 1;
	}

	// if we want to simplify a plan, just parse nothing
	if (convertPlan)
	{
		ifstream *plan = new ifstream(inputFiles[dfile]);
		ostream *outplan = &cout;
		if (pfile != -1)
		{
			ofstream *of = new ofstream(inputFiles[pfile]);
			if (!of->is_open())
			{
				cout << "I can't open " << inputFiles[pfile] << "!" << endl;
				return 2;
			}
			outplan = of;
		}

		convert_plan(*plan, *outplan);
		return 0;
	}

	if (pfile == -1 && !convertPlan)
	{
		cout << "You need to provide a domain and problem file as input." << endl;
		return 1;
	}

	// open c-style file handle
	FILE *domain_file = fopen(inputFiles[dfile].c_str(), "r");
	FILE *problem_file = fopen(inputFiles[pfile].c_str(), "r");

	if (!domain_file)
	{
		cout << "I can't open " << inputFiles[dfile] << "!" << endl;
		return 2;
	}
	if (!problem_file)
	{
		cout << "I can't open " << inputFiles[pfile] << "!" << endl;
		return 2;
	}
	if (!shopOutput && !hpdlOutput && !hddlOutput && !pureHddlOutput && !htn2stripsOutput && poutfile != -1)
	{
		cout << "For ordinary pandaPI output, you may only specify one output file, but you specified two: " << inputFiles[doutfile] << " and " << inputFiles[poutfile] << endl;
	}

	// parsing of command line arguments has been completed

	// parse the domain file
	run_parser_on_file(domain_file, (char *)inputFiles[dfile].c_str());
	run_parser_on_file(problem_file, (char *)inputFiles[pfile].c_str());

	if (showProperties)
	{
		printProperties();
		return 0;
	}

	if (pureHddlOutput || htn2stripsOutput)
	{
		// produce streams for output
		ostream *dout = &cout;
		ostream *pout = &cout;
		if (doutfile != -1)
		{
			ofstream *df = new ofstream(inputFiles[doutfile]);
			if (!df->is_open())
			{
				cout << "I can't open " << inputFiles[doutfile] << "!" << endl;
				return 2;
			}
			dout = df;
		}
		if (poutfile != -1)
		{
			ofstream *pf = new ofstream(inputFiles[poutfile]);
			if (!pf->is_open())
			{
				cout << "I can't open " << inputFiles[poutfile] << "!" << endl;
				return 2;
			}
			pout = pf;
		}
		if (pureHddlOutput)
			hddl_output(*dout, *pout, false, true, true, removeMethodPreconditions);
		else if (htn2stripsOutput)
			htn2strips_output(*dout, *pout);
		return 0;
	}

	if (!hpdlOutput)
		expand_sorts(); // add constants to all sorts

	// handle typeof-predicate
	if (!hpdlOutput && has_typeof_predicate)
		create_typeof();

	if (compileGoalIntoAction)
		compile_goal_into_action();
	if (removeMethodPreconditions)
		remove_method_preconditions();

	// do not preprocess the instance at all if we are validating a solution
	if (verifyPlan)
	{
		ifstream *plan = new ifstream(inputFiles[doutfile]);
		bool result = verify_plan(*plan, useOrderInPlanVerification, lenientVerify, verbosity);
		cout << "Plan verification result: ";
		if (result)
			cout << color(COLOR_GREEN, "true", MODE_BOLD);
		else
			cout << color(COLOR_RED, "false", MODE_BOLD);
		cout << endl;
		return result ? 0 : 1;
	}

	// TODO : BEFORE,
	if (!hpdlOutput)
	{
		// flatten all primitive tasks
		flatten_tasks(compileConditionalEffects, linearConditionalEffectExpansion, encodeDisjunctivePreconditionsInMethods);
		// .. and the goal
		flatten_goal();
		// create appropriate methods and expand method preconditions
		parsed_method_to_data_structures(compileConditionalEffects, linearConditionalEffectExpansion, encodeDisjunctivePreconditionsInMethods);
	}


	if (doVerifyEncoding)
	{
		encode_plan_verification(verificationFile);
	}

	if (shopOutput || hpdlOutput)
	{
		// produce streams for output
		ostream *dout = &cout;
		ostream *pout = &cout;
		if (doutfile != -1)
		{
			ofstream *df = new ofstream(inputFiles[doutfile]);
			if (!df->is_open())
			{
				cout << "I can't open " << inputFiles[doutfile] << "!" << endl;
				return 2;
			}
			dout = df;
		}
		if (poutfile != -1)
		{
			ofstream *pf = new ofstream(inputFiles[poutfile]);
			if (!pf->is_open())
			{
				cout << "I can't open " << inputFiles[poutfile] << "!" << endl;
				return 2;
			}
			pout = pf;
		}
		if (shopOutput)
			write_instance_as_SHOP(*dout, *pout);
		if (hpdlOutput)
			write_instance_as_HPDL(*dout, *pout);
		return 0;
	}

	// split methods with independent parameters to reduce size of grounding
	if (splitParameters)
		split_independent_parameters();
	// cwa, but only if we actually want to compile negative preconditions
	if (!hpdlOutput || internalHDDLOutput)
		compute_cwa();
	// simplify constraints as far as possible
	reduce_constraints();
	clean_up_sorts();
	remove_unnecessary_predicates();

	linearize(removeMethodPreconditions, doutfile, poutfile);
	// produce streams for output (for hddl, not the sat-ish output typically given)
	// produce streams for output
	ofstream dout2;
	ofstream pout2; 
	string dout_hddl_name = inputFiles[doutfile];
	string pout_hddl_name = inputFiles[poutfile];
	dout2.open(dout_hddl_name);
	pout2.open(pout_hddl_name);
	//                       Output, usedParsed, dontWriteConstantsIntoDomain
	hddl_output(dout2, pout2, false,   false,    true, removeMethodPreconditions);
	printf("Finished writing.\n");
	dout2.close();
	pout2.close();

	// // write to output
	// if (verboseOutput)
	// 	verbose_output(verbosity);
	// else if (hddlOutput)
	// {
	// 	// produce streams for output
	// 	ostream *dout = &cout;
	// 	ostream *pout = &cout;
	// 	if (doutfile != -1)
	// 	{
	// 		ofstream *df = new ofstream(inputFiles[doutfile]);
	// 		if (!df->is_open())
	// 		{
	// 			cout << "I can't open " << inputFiles[doutfile] << "!" << endl;
	// 			return 2;
	// 		}
	// 		dout = df;
	// 	}
	// 	if (poutfile != -1)
	// 	{
	// 		ofstream *pf = new ofstream(inputFiles[poutfile]);
	// 		if (!pf->is_open())
	// 		{
	// 			cout << "I can't open " << inputFiles[poutfile] << "!" << endl;
	// 			return 2;
	// 		}
	// 		pout = pf;
	// 	}
	// 	hddl_output(*dout, *pout, internalHDDLOutput, false, dontWriteConstantsIntoDomain, false);
	// }
	// else
	// {
	// 	ostream *dout = &cout;
	// 	if (doutfile != -1)
	// 	{
	// 		ofstream *df = new ofstream(inputFiles[doutfile]);
	// 		if (!df->is_open())
	// 		{
	// 			cout << "I can't open " << inputFiles[doutfile] << "!" << endl;
	// 			return 2;
	// 		}
	// 		dout = df;
	// 	}
	// 	simple_hddl_output(*dout);
	// }
}
