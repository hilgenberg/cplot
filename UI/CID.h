#pragma once

enum class CID // CommandID
{
	RETURN, // args are the return values
	ERROR,  // arg0 is the error string
	READ,   // arg0 is the file path
	LS,     // arg0='p' for parameters, etc.
	ANIM,   // args: p, v0, v1, dt, type, reps
	STOP,   // args: [p]
	GET,    // arg0: what to return (internal command)
	SET,    // args: prop [value] (not internal!)
	CG,     // [arg0]: graph number
	GRAPH,  // args: comp_idx (0 for all), f1, ...
	PARAM,  // args: pname, value
	FOCUS,  // args: none
};

enum class GET // things that the GET command can return
{
	PARAMETER_NAMES,
	USED_PARAMETER_NAMES,
	GRAPH_COUNT,
	CURRENT_GRAPH_EXPRESSIONS,
	PROPERTY_NAMES,
	PROPERTY_VALUES, // arg1: property name
};
