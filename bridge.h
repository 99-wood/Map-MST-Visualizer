#ifndef BRIDGE_H
#define BRIDGE_H

#include <QObject>
#include <qdebug.h>

class Bridge : public QObject
{
    Q_OBJECT
private slots:
    void debug(double x, double y){
        qDebug() << QString("%1").arg(x, 0, 'g', 30) << " " << QString("%1").arg(y, 0, 'g', 30) << endl;
    }
public:
    explicit Bridge(QObject *parent = nullptr);

public slots:
    void print(QString s) const {
        qDebug() << s << endl;
    }
    void toQt(double x, double y){
        emit receiveInfo(x, y);
    }
signals:
    void receiveInfo(double x, double y);
};

#endif // BRIDGE_H
