#include <sstream>
#include <iterator>
#include <utility>
#include "scene.h"
#include "filter.h"

using namespace std;

void concat_filter_input::combine(const input_table &inputs) {
	input_table::const_iterator i;
	for (i = inputs.begin(); i != inputs.end(); ++i) {
		filter_param_set *p;
		filter_result *r = i->res;

		for (int j = r->first_added(); j < r->num_current(); ++j) {
			p = new filter_param_set();
			(*p)[i->name] = r->get_current(j);
			val2params[r->get_current(j)] = p;
			add(p);
		}
		for (int j = 0; j < r->num_removed(); ++j) {
			if (!map_pop(val2params, r->get_removed(j), p)) {
				assert(false);
			}
			remove(p);
		}
		for (int j = 0; j < r->num_changed(); ++j) {
			p = val2params[r->get_changed(j)];
			change(p);
		}
	}
}

void product_filter_input::combine(const input_table &inputs) {
	input_table::const_iterator i;
	for (i = inputs.begin(); i != inputs.end(); ++i) {
		val2param_map::iterator k;
		param_set_list::iterator l;
		filter_result *r = i->res;
		
		for (int j = 0; j < r->num_removed(); ++j) {
			k = val2params.find(r->get_removed(j));
			assert(k != val2params.end());
			for (l = k->second.begin(); l != k->second.end(); ++l) {
				remove(*l);
				erase_param_set(*l);
			}
		}
		for (int j = 0; j < r->num_changed(); ++j) {
			k = val2params.find(r->get_changed(j));
			assert(k != val2params.end());
			for (l = k->second.begin(); l != k->second.end(); ++l) {
				change(*l);
			}
		}
	}
	gen_new_combinations(inputs);
}

class product_gen {
public:
	product_gen(const vector<int> &begin, const vector<int> &end)
	: begin(begin), curr(begin), end(end), i(0), first(true) {}
	
	bool next() {
		if (first) {
			for (int i = 0; i < begin.size(); ++i) {
				if (begin[i] == end[i]) {
					return false;
				}
			}
			first = false;
			return true;
		}
		
		while (true) {
			if (++curr[i] == end[i]) {
				if (i == curr.size() - 1) {
					return false;
				}
				curr[i] = begin[i];
			} else {
				return true;
			}
			if (++i >= curr.size()) {
				i = 0;
			}
		}
	}
	
	vector<int> curr;

private:
	vector<int> begin;
	vector<int> end;
	int i;
	bool first;
};

/*
 Generate all combinations of results that involve at least one new
 result.  Do this by iterating over the result lists.  For the i^th
 result list, take the cartesian product of the old results from lists
 0..(i-1), the new results of list i, and both old and new results from
 lists (i+1)..n.  This will avoid generating duplicates.  I'm assuming
 that new results are at the end of each result list.
*/
void product_filter_input::gen_new_combinations(const input_table &inputs) {
	for (int i = 0; i < inputs.size(); ++i) {
		vector<int> begin, end;
		for (int j = 0; j < inputs.size(); ++j) {
			if (j < i) {
				begin.push_back(0);
				end.push_back(inputs[j].res->first_added()); // same as end of old results
			} else if (j == i) {
				begin.push_back(inputs[j].res->first_added());
				end.push_back(inputs[j].res->num_current());
			} else {
				begin.push_back(0);
				end.push_back(inputs[j].res->num_current());
			}
		}
		product_gen gen(begin, end);
		while (gen.next()) {
			filter_param_set *p = new filter_param_set();
			for (int j = 0; j < inputs.size(); ++j) {
				filter_val *v = inputs[j].res->get_current(gen.curr[j]);
				(*p)[inputs[j].name] = v;
				val2params[v].push_back(p);
			}
			add(p);
		}
	}
}

void product_filter_input::erase_param_set(filter_param_set *s) {
	filter_param_set::const_iterator i;
	for (i = s->begin(); i != s->end(); ++i) {
		param_set_list &l = val2params[i->second];
		l.erase(find(l.begin(), l.end(), s));
	}
}

filter_input::~filter_input() {
	input_table::iterator i;
	for (i = input_info.begin(); i != input_info.end(); ++i) {
		delete i->f;
	}
}

bool filter_input::update() {
	input_table::iterator i;
	for (i = input_info.begin(); i != input_info.end(); ++i) {
		if (!i->f->update()) {
			return false;
		}
	}

	combine(input_info);

	for (i = input_info.begin(); i != input_info.end(); ++i) {
		i->res->clear_changes();
	}
	
	return true;
}

void filter_input::add_param(string name, filter *f) {
	param_info i;
	i.name = name;
	i.f = f;
	i.res = f->get_result();
	input_info.push_back(i);
}
