#include <vector>
#include <string>
#include <bits/stdc++.h>

static int getMin(int a, int b, int c)
{
	if (a < b && a < c)
		return a;
	if (b < a && b < c)
		return b;
	return c;
}

namespace Utils
{
	// https://en.wikipedia.org/wiki/Levenshtein_distance
	int LevenshteinDistance(const std::string& s1, const std::string& s2)
	{
		int m = s1.size();
		int n = s2.size();
	
		std::vector<int> v0(n + 1);
		std::vector<int> v1(n + 1);
	
		for (int j = 0; j <= n; j++)
			v0[j] = j;
	
		for (int i = 0; i < m; i++)
		{
			v1[0] = i + 1;
			for (int j = 0; j < n; j++)
			{
				int cost = (s1[i] == s2[j]) ? 0 : 1;
				v1[j + 1] = getMin(v0[j + 1] + 1, v1[j] + 1, v0[j] + cost);
			}
			std::swap(v0, v1);
		}
		return v0[n];
	}

}	

