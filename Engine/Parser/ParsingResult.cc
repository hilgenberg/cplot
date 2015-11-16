#include "ParsingResult.h"
#include "../Namespace/Expression.h"

void ParsingResult::print(const Expression &ex) const
{
	printf("%s. At this point:\n", info.c_str());
	auto F = ex.strings();
	if (index >= F.size())
	{
		assert(false);
		printf("Invalid error source index (This should not happen!)\n");
		return;
	}
	printf("%s\n%*s", F[index].c_str(), (int)pos, "");
	if (len == 0)
	{
		printf("^--\n");
	}
	else
	{
		for (size_t i = 0; i < len; ++i) putchar('~');
		putchar('\n');
	}
}

