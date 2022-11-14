#pragma once
#include <deque>
#include "../Utility/Timer.h" // for now()

// Note: Never ever put any pointers or references onto the undo stack unless you
// know for sure that their lifetime is at least as long as the undo stack itself!
// So the document itself (which contains the undo tracker) is fine. As are permanent
// global objects and few other things. Everything else needs to be stored as an OID
// or serialized! That goes for captured "this" pointers too!

// Note: Saving a file resets the change counter to zero, but leaves the undo and redo
// stacks alive. So one could:
// 0. new file
// 1. change something (changes == 1, one item on undo stack)
// 2. save file (changes == 0, still one item on the undo stack)
// 3. undo (changes == -1, undo stack empty, one on redo stack)
// 4. change something (clears the redo stack)
//    --> this must not set changes to zero again because it is not the saved state!
//    --> changes can never become zero again unless you save or load, because
//        there is no way back to the saved state!
// That is what UndoTracker::frozen_change_counter is for - it marks the situation
// that there is no way back to the saved state (at least via undo, you can always
// load the saved file, obviously).

// Note: Our menu item capitalization is like "Do That Thing", so the undo names
// should be all title-case (Capitalized "Undo"/"Redo" gets prepended automatically - 
// you pass "That Thing" and get "Undo That Thing" in the menu).

// Note: When continuously modifying the same thing (dragging a slider that sets some
// parameter value f.e.), you can pass a pointer to the modified thing (here you need
// not worry about object lifetime!) to drop all but the first undo item (which must
// be like "set thing to value" and not "decrement thing").
// Toggling a boolean is different though - here we can only drop the undo items in
// pairs - in that case pass TOGGLE_OP into UndoTracker::reg(... op_id).
constexpr int TOGGLE_OP = INT_MIN;

class UndoTracker
{
public:
	typedef std::function<void(void)> Action;

	UndoTracker()
	: undoing(false), redoing(false), changes(0), all_trivial(true)
	, last_coalesce_token(NULL), last_op_id(0), num_toggles(0), last_reg_time(-1)
	{}

	bool have_changes(bool non_trivial = true) const
	{
		return changes != 0 && (!non_trivial || !all_trivial);
		// equivalent:
		//return non_trivial ? (changes != 0 && !all_trivial) : changes != 0;
	}

	void reg(std::string name, Action a, void *coalesce_token = NULL, int op_id = 0, bool trivial = false)
	{
		assert(!undoing || !redoing); // never both
		assert(!undoing || !us.empty()); // items are always popped only after calling them!
		assert(!redoing || !rs.empty()); // items are always popped only after calling them!

		auto &tgt = undoing ? rs : us;
		if (undoing) name = us.back().name;
		if (redoing) name = rs.back().name;
		if (!undoing && !redoing)
		{
			if (!trivial) all_trivial = false;

			if (changes < 0)
				changes = frozen_change_counter;
			else
				++changes;

			double t = now(); // don't coalesce after pauses

			// does this change match the last one?
			if (coalesce_token && coalesce_token == last_coalesce_token &&
			    op_id == last_op_id && last_reg_time > 0 && t < last_reg_time + 30.0 &&
			    !us.empty())
			{
				if (op_id == TOGGLE_OP)
				{
					++num_toggles;
					if (num_toggles >= 3)
					{
						// remove two toggles from three, leave one
						num_toggles -= 2;
						if (changes != frozen_change_counter) changes -= 2;
						us.pop_back();
					}
					else
					{
						// two matching toggles, but no more - leave them
						tgt.emplace_back(name, a);
					}
				}
				else
				{
					// matching changes, undo them in a single step
					num_toggles = 0;
					if (changes != frozen_change_counter) --changes;
				}
			}
			else
			{
				// no match, just add it
				num_toggles = (op_id == TOGGLE_OP);
				tgt.emplace_back(name, a);
			}
			last_coalesce_token = coalesce_token;
			last_op_id = op_id;
			last_reg_time = t;

			rs.clear();
		}
		else
		{
			if (redoing && !trivial) all_trivial = false;
			if (undoing && changes <= 0) all_trivial = false;
			last_coalesce_token = NULL; // never coalesce through undo/redo
			tgt.emplace_back(name, a);
			if (changes != frozen_change_counter) undoing ? --changes : ++changes;
		}	
	}

	void file_was_saved() const // const so that Document::save() can be const
	{
		changes = 0; all_trivial = true;
		last_coalesce_token = NULL;
	}
	void file_was_loaded()
	{
		us.clear();
		rs.clear();
		changes = 0; all_trivial = true;
		last_coalesce_token = NULL;
	}

	bool can_undo(std::string &name) const
	{
		assert(!undoing && !redoing);
		if (us.empty()) return false;
		name = "Undo " + us.back().name;
		return true;
	}
	bool can_redo(std::string &name) const
	{
		assert(!undoing && !redoing);
		if (rs.empty()) return false;
		name = "Redo " + rs.back().name;
		return true;
	}

	#ifndef NDEBUG
	// #define CHECK_REDO_REG
	#endif
	// this fails assertions when e.g. moving some slider somewhere and back,
	// which makes the redo-operation a no-op, which is not registered.

	void undo()
	{
		assert(!undoing && !redoing);
		if (us.empty()) return;
		#ifdef CHECK_REDO_REG
		size_t n0 = rs.size();
		#endif
		UndoItem &item = us.back();
		undoing = true;
		try {
			item.action();
		}
		catch (...)
		{
			undoing = false;
			us.pop_back();
			throw;
		}
		undoing = false;
		assert(&item == &us.back()); // undo only modifies redo stack
		#ifdef CHECK_REDO_REG
		assert(rs.size() > n0); // should have gone from undo to redo
		#endif
		us.pop_back();
	}
	void redo()
	{
		assert(!undoing && !redoing);
		if (rs.empty()) return;
		#ifdef CHECK_REDO_REG
		size_t n0 = us.size();
		#endif
		UndoItem &item = rs.back();
		redoing = true;
		try {
			item.action();
		}
		catch (...)
		{
			redoing = false;
			rs.pop_back();
			throw;
		}
		redoing = false;
		assert(&item == &rs.back()); // redo only modifies undo stack
		#ifdef CHECK_REDO_REG
		assert(us.size() > n0); // should have gone from redo to undo
		#endif
		rs.pop_back();
	}

private:
	struct UndoItem
	{
		UndoItem(const std::string &name, Action a) : action(a), name(name) {}
		
		std::string name;
		Action      action;
	};

	std::deque<UndoItem> us, rs;
	bool undoing, redoing;
	mutable int    changes; // number of changes relative to the loaded/saved/new file
	mutable bool   all_trivial;

	mutable void  *last_coalesce_token;
	mutable int    last_op_id, num_toggles;
	mutable double last_reg_time;

	static constexpr int frozen_change_counter = INT_MIN;
};

