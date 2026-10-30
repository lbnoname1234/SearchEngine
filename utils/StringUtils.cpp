#include "StringUtils.h"
#include <vector>
#include <algorithm>

int StringUtils::levenshteinDistance(const string &a, const string &b)
{
    int m = a.size(), n = b.size();
    vector<vector<int>> dp(m + 1, vector<int>(n + 1));

    for (int i = 0; i <= m; i++)
        dp[i][0] = i;
    for (int j = 0; j <= n; j++)
        dp[0][j] = j;

    for (int i = 1; i <= m; i++)
    {
        for (int j = 1; j <= n; j++)
        {
            dp[i][j] = min({
                dp[i - 1][j] + 1,                         // xóa
                dp[i][j - 1] + 1,                         // thêm
                dp[i - 1][j - 1] + (a[i - 1] != b[j - 1]) // thay thế
            });
        }
    }
    return dp[m][n];
}