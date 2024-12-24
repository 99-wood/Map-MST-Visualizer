#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    bridge = new Bridge(this);
    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject("bridge", bridge);

    ui->mapWidget->page()->setWebChannel(channel);

    ui->mapWidget->page()->load(QUrl("qrc:/map.html"));

//    connect(bridge, &Bridge::receiveInfo, this, [&](double x, double y){
//        ui->mapWidget->page()->runJavaScript(QString("drawLine(%1,%2,%3,%4)").arg(123.431).arg(41.6591).arg(x, 0, 'g', 30).arg(y, 0, 'g', 30));
//    });
    connect(ui->cleanButton, &QPushButton::clicked, this, &MainWindow::cleanMap);
//    connect(ui->calButton, &QPushButton::clicked, this, &MainWindow::cleanLine);
    connect(ui->calButton, &QPushButton::clicked, this, &MainWindow::getPoint);
    connect(bridge, &Bridge::receiveInfo, this, &MainWindow::input);
    connect(this, &MainWindow::inputFinish, this, &MainWindow::calc);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::cleanMap()
{
    qDebug("clean");
    ui->mapWidget->page()->runJavaScript(QString("clean()"));
}

void MainWindow::cleanLine()
{
    qDebug("cleanLine");
    ui->mapWidget->page()->runJavaScript(QString("cleanLine()"));

}

void MainWindow::getPoint()
{
    qDebug("getPoint");
    points.clear();
    ui->mapWidget->page()->runJavaScript(QString("getPoint()"));
}

void MainWindow::input(double x, double y)
{
    if(x < 0) emit this->inputFinish();
    else points.push_back(qMakePair(x, y));
    return;
}

void MainWindow::calc()
{
    cleanLine();
    dsu checker(points.size());
    using Point = QPair<double, double>;
    QVector<QPair<int, int>> q;
    auto dis = [&](Point a, Point b) -> double {
//        return sqrt((a.first - b.first) * (a.first - b.first) + (a.second - b.second) * (a.second - b.second));
        const double R = 6371.0; // 地球半径，单位：千米

        // 将角度转换为弧度
        double lat1 = a.second * M_PI / 180.0; // 纬度a
        double lon1 = a.first * M_PI / 180.0;  // 经度a
        double lat2 = b.second * M_PI / 180.0; // 纬度b
        double lon2 = b.first * M_PI / 180.0;  // 经度b

        // 计算差值
        double dlat = lat2 - lat1;
        double dlon = lon2 - lon1;

        // 哈弗辛公式
        double h = sin(dlat / 2) * sin(dlat / 2) +
                   cos(lat1) * cos(lat2) * sin(dlon / 2) * sin(dlon / 2);
        double distance = 2 * R * atan2(sqrt(h), sqrt(1 - h));

        return distance;
    };
    for(int i = 0; i < points.size(); ++i){
        for(int j = 0; j < i; ++j){
            q.push_back(qMakePair(i, j));
        }
    }
    std::sort(q.begin(), q.end(), [&](QPair<int, int> a, QPair<int, int> b){
      return dis(points[a.first], points[a.second]) < dis(points[b.first], points[b.second]);
    });
    QVector<QPair<int, int>> ans;
    double all = 0;
    for(auto tmp : q){
        auto x = tmp.first, y = tmp.second;
        if(checker.find(x) == checker.find(y)) continue;
        checker.merge(x, y);
        ans.push_back(qMakePair(x, y));
        all += dis(points[x], points[y]);
    }
    ui->label->setText(QString("全长: %1 km").arg(all, 0, 'g', 5));
    for(auto tmp : ans){
        auto x = tmp.first, y = tmp.second;
        ui->mapWidget->page()->runJavaScript(QString("drawLine(%1,%2,%3,%4)")
                                             .arg(points[x].first, 0, 'g', 30).arg(points[x].second, 0, 'g', 30)
                                             .arg(points[y].first, 0, 'g', 30).arg(points[y].second, 0, 'g', 30));
    }
    points.clear();
}

