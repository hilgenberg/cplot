#include "ObjectDB.h"
#include "../../Utility/Mutex.h"

#include <map>
#include <set>
#include <cassert>
#include <iostream>

class ObjectDB
{
public:
	ObjectDB(const ObjectDB &) = delete;
	ObjectDB &operator=(const ObjectDB &) = delete;

	ObjectDB() : next_oid(1){ }
	~ObjectDB(){ dump(); }
	
	inline void assign_new_id(IDCarrier *x)
	{
		assert(x);
		if (!x) return;
		
		Lock lock(mutex);
		
		IDCarrier::OID oid = next_oid++;

		assert(deleted.count(oid) == 0);
		assert(objects.count(oid) == 0);
		
		x->m_oid = oid;
		objects[oid] = x;
	}

	inline void remove(IDCarrier *x)
	{
		assert(x);
		if (!x) return;

		Lock lock(mutex);
		
		#ifndef NDEBUG
		assert(x->m_oid < next_oid);
		assert(objects.count(x->m_oid) == 1);
		assert(deleted.count(x->m_oid) == 0);
		deleted.insert(x->m_oid);
		#endif
		
		objects.erase(x->m_oid);
	}

	inline void restore(IDCarrier *x, IDCarrier::OID oid)
	{
		assert(x);
		if (!x) return;

		Lock lock(mutex);
		
		#ifndef NDEBUG
		assert(deleted.count(oid) == 1);
		assert(objects.count(x->m_oid) == 1);
		assert(objects.count(oid) == 0);
		assert(oid < next_oid);
		deleted.erase(oid);
		#endif
		
		objects.erase(x->m_oid);
		x->m_oid = oid;
		objects[oid] = x;
	}
	
	IDCarrier *find(IDCarrier::OID oid) const
	{
		auto i = objects.find(oid);
		//assert(i != objects.end());
		return (i == objects.end() ? NULL : i->second);
	}

	void dump() const
	{
		#ifdef ODBDEBUG
		using std::cerr;
		using std::endl;
		
		cerr << endl << "State of ObjectDB " << this << endl;
		cerr << "----------------------------------------------------" << endl;
		for (auto &i : objects)
		{
			cerr << i.first << " --> " << i.second << ": ";
			i.second->dump(cerr);
			cerr << (i.second->m_oid == i.first ? "" : " MISMATCH!") << endl;
		}
		cerr << "Next ID: " << next_oid << endl;
		cerr << "----------------------------------------------------" << endl;
		#endif
	}

private:
	IDCarrier::OID next_oid;
	std::map<IDCarrier::OID, IDCarrier*> objects;
	#ifndef NDEBUG
	std::set<IDCarrier::OID> deleted;
	#endif
	
	Mutex mutex;
};

static ObjectDB _db;

//----------------------------------------------------------------------------------------------------------------------

IDCarrier:: IDCarrier(){ _db.assign_new_id(this); }
IDCarrier::~IDCarrier(){ _db.remove(this); }
	
void IDCarrier::restore(IDCarrier::OID old_id){ _db.restore(this, old_id); }
IDCarrier *IDCarrier::find(IDCarrier::OID oid){ return _db.find(oid); }

