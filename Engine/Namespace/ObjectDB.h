#pragma once

#include <cstdint>
#ifdef DEBUG
#include <iostream>
#endif

/**
 * Utility class to handle undo cases like modify -> delete -> undo delete -> undo modify
 * where the object is deleted and recreated, which would crash the undo modify if it passed the pointer.
 */

class IDCarrier
{
public:
	typedef uint64_t OID; ///< Object ID, will be unique over the runtime of the program

	static IDCarrier *find(OID oid); ///< @return The object with the oid or NULL if it does not or no longer exist.
	
	IDCarrier();  ///< Assigns a unique ID
	~IDCarrier(); ///< Marks the ID as deleted (nonvirtual, so never delete an IDCarrier*)

	void restore(OID old_id); ///< Restores the OID after undoing deletion for an object.
	OID oid() const{ return m_oid; }
	
	#ifdef DEBUG
	virtual void dump(std::ostream &o) const = 0;
	#endif
	
private:
	OID m_oid;
	friend class ObjectDB;
};
