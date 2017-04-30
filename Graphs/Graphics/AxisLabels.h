#pragma once
#include <string>
#include <vector>
#include <cassert>
class GridIterator;
class LabelIterator;

class AxisLabels
{
public:
	AxisLabels(double pixel, double range);
	bool valid() const{ return !invalid; }
	
private:
	friend class GridIterator;
	friend class LabelIterator;

	double major, minor, label;
	int subdiv; // number of minor ticks between major ones + 1
	bool invalid;
};

class GridIterator
{
public:
	GridIterator(const AxisLabels &a, double min, double max);
	bool done() const{ return i >= n; }
	void operator++(   ){ ++i; assert(i <= n); }
	void operator++(int){ ++i; assert(i <= n); }
	double   x() const{ return x0 + a.minor * i; }
	bool is_minor() const{ return (i+sub) % a.subdiv != 0; }

private:
	const AxisLabels &a;
	int i, n, sub; // i = current index, n = total, sub = subdivision index for #0 (see minor())
	double x0; // first grid line
};

class LabelIterator
{
public:
	LabelIterator(const AxisLabels &a, double min, double max, double char_size); // char_size should be 0 for vertically stacked text
	bool done() const{ return i >= n || n > 100; }
	void operator++(   ){ ++i; assert(i <= n); }
	void operator++(int){ ++i; assert(i <= n); }
	double   x() const{ return x0 + a.label * i; }
	std::string label() const; // will be empty for left out places (which should still get a tick but no text)
	bool skipped() const{ return (i+sub) % skip != 0; }
	int max_len() const{ return max_label_len; }

private:
	const AxisLabels &a;
	int i, n, sub, skip; // i = current index, n = total, sub = skipping index for #0
	double x0; // first label
	int max_label_len;
};
