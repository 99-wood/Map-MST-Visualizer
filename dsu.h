#ifndef DSU_H
#define DSU_H
#include <QVector>

class dsu
{
private:
    const int n;
    QVector<int> fa;
public:
    dsu(int n);
    int find(int p);
    void merge(int x, int y);
};

#endif // DSU_H
