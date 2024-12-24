#include "dsu.h"

dsu::dsu(int n) : n(n), fa(n)
{
    for(int i = 0; i < n; ++i){
        fa[i] = i;
    }
    return;
}

int dsu::find(int p)
{
    return fa[p] == p ? p : fa[p] = find(fa[p]);
}

void dsu::merge(int x, int y)
{
    assert(x < n && y < n && x >= 0 && y >= 0);
    int fx = fa[x], fy = fa[y];
    if(fx == fy) return;
    fa[fx] = fy;
    return;
}
