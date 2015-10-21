#pragma once
#include <atomic>
#include <thread>
#include <pthread.h>
#include <vector>
#include "../Utility/System.h"
#include "main.h"
#include "CID.h"

struct Argument
{
	Argument(int i) : type(I), i(i){}
	Argument(double d) : type(D), d(d){}
	Argument(const std::string &s) : type(S), s(s){}

	enum Type{ I, D, S };
	Type type;
	union
	{
		int i;
		double d;
	};
	std::string s;
};

class Command
{
public:
	// Usage is:
	// CLI: send(...), read reply
	// GUI: check bool(), read command data, write reply, done() or error()

	Command() : todo(false), cid(CID::ERROR){ }

	operator bool()
	{
		if (!todo.load(std::memory_order_relaxed)) return false;
		atomic_thread_fence(std::memory_order_acquire);
		return true;
	}

	bool send(CID c, std::vector<Argument> &&a)
	{
		cid = c;
		args = std::move(a);
		fflush(stdout);
		state(true);
		while (*this && !quit){ pthread_yield(); sleep(0.01); }
		return cid == CID::RETURN;
	}
	bool send(CID c, const Argument &a)
	{
		std::vector<Argument> args(1, a);
		return send(c, std::move(args));
	}
	bool send(CID c)
	{
		std::vector<Argument> args;
		return send(c, std::move(args));
	}


	void done(){ fflush(stdout); state(false); }
	void error(const std::string &desc)
	{
		cid = CID::ERROR;
		args.clear();
		args.emplace_back(desc);
		done();
	}
	
	CID cid;
	std::vector<Argument> args;

	inline Argument &get_arg(size_t i, bool last = false)
	{
		size_t na = args.size();
		if (i >= na || last && i+1 < na) throw std::logic_error("Wrong number of arguments");
		return args[i];
	}

	inline Argument &get_arg(Argument::Type t, size_t i, bool last = false)
	{
		Argument &a = get_arg(i, last);
		if (a.type != t) throw std::logic_error("Wrong argument type");
		return a;
	}
	inline std::string &get_str  (size_t i, bool last = false){ return get_arg(Argument::S, i, last).s; }
	inline int          get_int  (size_t i, bool last = false){ return get_arg(Argument::I, i, last).i; }

private:
	std::atomic<bool> todo;

	void state(bool what)
	{
		atomic_thread_fence(std::memory_order_release);
		todo.store(what, std::memory_order_relaxed);
	}
};

extern Command cmd; // we need only one

