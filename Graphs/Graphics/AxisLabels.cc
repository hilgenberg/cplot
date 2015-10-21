#include "AxisLabels.h"
#include "../../Utility/StringFormatting.h"
#include <cmath>

//----------------------------------------------------------------------------------------------------------------------
//  AxisLabels
//----------------------------------------------------------------------------------------------------------------------

AxisLabels::AxisLabels(double pixel, double range) // label spacing is optimized for range
{
	invalid = true; // if for some reason we can't find usable positions
	
	// we have a hierarchy of natural grids, every ..100, 10, 1, 0.1, 0.01, ...
	
	// the major ticks are always one of those (one that fits the pixel size)
	double spc0  = 30.0 * pixel;
	major = pow(10.0, ceil(log10(spc0)));
	if (major == 0.0 || !finite(major)) return;
	assert(major >= spc0);
	assert(major <= 300.0 * pixel);
	
	// the minor ticks are a subset of the next lower level (every one or every 2nd or none at all)
	double spc1 = 6.0 * pixel;
	if (major*0.1 >= spc1)
	{
		minor = 0.1*major;
		subdiv = 10;
	}
	else if (major*0.2 >= spc1)
	{
		minor = 0.2*major;
		subdiv = 5;
	}
	else // no minor grid lines
	{
		minor = major;
		subdiv = 1;
	}
	if (minor == 0.0 || !finite(minor)) return;
	assert(minor >= spc1);
	assert(minor <= 300.0 * pixel);

	// for labels we start with one on every major tick, which can be too much or too little
	// we want somewhere between say 6 and 20 labels on the range
	label = major;
	if      (range > 20 * label*10.0){ label *= 10.0; }
	else if (range > 20 * label* 5.0){ label *=  5.0; }
	else if (range > 20 * label* 2.0){ label *=  2.0; }
	else if (range <  6 * label* 0.1){ label *=  0.1; }
	else if (range <  6 * label* 0.2){ label *=  0.2; }
	else if (range <  6 * label* 0.5){ label *=  0.5; }
	
	// if the labels are written horizontally, this might be a subset again, so they don't overlap
	// LabelIterator handles that
	
	invalid = false;
}

//----------------------------------------------------------------------------------------------------------------------
//  GridIterator
//----------------------------------------------------------------------------------------------------------------------

GridIterator::GridIterator(const AxisLabels &a, double min, double max) : a(a), i(0)
{
	assert(a.valid());
	
	x0 = fmod(min, a.minor);
	double xm = fmod(min, a.major);
	if (x0 < 0.0) x0 += a.minor;
	if (xm < 0.0) xm += a.major;
	assert(x0 >= 0.0 && x0 < a.minor);
	assert(xm >= 0.0 && xm < a.major);
	x0 = min + (a.minor - x0);
	xm = min + (a.major - xm);
	sub = a.subdiv - int(round((xm - x0) / a.minor));
	if (sub < 0) sub += a.subdiv;
	
	n = (int) floor(1.0 + (max-x0) / a.minor);
}

//----------------------------------------------------------------------------------------------------------------------
//  LabelIterator
//----------------------------------------------------------------------------------------------------------------------

static int label_length(double x, double dx)
{
	/// @todo (in LabelIterator::LabelIterator() because doxygen skips static functions)

	if (dx < 1.0)
	{
		double d = pow(10.0, floor(log10(dx)));
		double v = d * round(x / d);
		return (int)format("%.12g", v).length();
	}
	else
	{
		return (int)format("%g", round(x)).length();
	}
}

#include <iostream>
std::string LabelIterator::label() const
{
	if (skipped()) return "";
	
	if (a.label < 1.0)
	{
		double d = pow(10.0, floor(log10(a.label)));
		double v = d * round(x() / d);
		return format("%.12g", v);
	}
	else
	{
		return format("%g", round(x()));
	}
}

LabelIterator::LabelIterator(const AxisLabels &a, double min, double max, double char_size) : a(a), i(0)
{
	x0 = fmod(min, a.label);
	if (x0 < 0.0) x0 += a.label;
	assert(x0 >= 0.0 && x0 < a.label);
	x0 = min + (a.label - x0);
	n = (int) floor(1.0 + (max-x0) / a.label);

	skip = 1;

	if (char_size <= 0.0)
	{
		sub  = 0;
		max_label_len = 0;
	}
	else
	{
		max_label_len = 0;
		for (int j = 0; j < n; ++j)
		{
			/// @todo calculate label_length() directly (without using format(...))

			int len = label_length(x0 + j*a.label, a.label);
			if (len > max_label_len) max_label_len = len;
		}
		if (char_size * (max_label_len+0.75) > a.label) skip = 2;
		
		if (skip > 1)
		{
			double xm = fmod(min, skip*a.label);
			if (xm < 0.0) xm += skip*a.label;
			assert(xm >= 0.0 && xm < skip*a.label);
			xm = min + (skip*a.label - xm);
			sub = skip - int(round((xm - x0) / a.label));
			if (sub < 0) sub += skip;
		}
		else
		{
			sub = 0;
		}
	}
}
