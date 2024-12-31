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
    connect(ui->importButton, &QPushButton::clicked, this, &MainWindow::importFile);
    connect(ui->exportButton, &QPushButton::clicked, this, &MainWindow::exportFile);
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

void MainWindow::addPoint(double x, double y)
{
    ui->mapWidget->page()->runJavaScript(QString("drawPoint(%1,%2)")
                                         .arg(x, 0, 'g', 30).arg(y, 0, 'g', 30));
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

void MainWindow::importFile()
{
//    qDebug() << 6;

    // 打开目录选择对话框
     QString fileName = QFileDialog::getOpenFileName(this, tr("Select Text File"), "", tr("Text Files (*.txt)"));

    // 如果用户选择了目录，将路径设置到文本框中
    if (fileName.isEmpty()) {
        QMessageBox::critical(this, tr("ERROR"),  tr("Wrong File!"));
        return;
    }
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file:" << file.errorString();
        return; // 返回空列表表示失败
    }
    cleanMap();
    QTextStream in(&file); // 创建文本流
    QString line = in.readLine(); // 逐行读取
    QStringList parts = line.split(" ", QString::SkipEmptyParts); // 按空格分隔
    int n = parts[0].toInt();
    for(int i = 0; i < n; ++i){
        line = in.readLine();
        QStringList parts = line.split(" ", QString::SkipEmptyParts); // 按空格分隔
        double x = parts[0].toDouble(), y = parts[1].toDouble();
        addPoint(x, y);
    }
    return;
}

void MainWindow::exportFile() {
    QString filePath = QFileDialog::getSaveFileName(this, "选择保存位置\n", "", "Text Files (*.txt);;All Files (*)");

    // 如果用户没有选择文件，直接返回
    if (filePath.isEmpty()) {
        return;
    }

    // 检查文件是否已经存在
    QFile file(filePath);
//    if (file.exists()) {
//        // 文件已存在，询问用户是否覆盖
//        QMessageBox::StandardButton reply;
//        reply = QMessageBox::question(this, "文件已存在\n", "文件已存在，是否覆盖？\n",
//                                      QMessageBox::Yes | QMessageBox::No);
//        if (reply == QMessageBox::No) {
//            return;  // 用户选择不覆盖，退出函数
//        }
//    }

    // 创建并打开文件，准备写入
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("ERROR"),  tr("Can not open the file!"));
        return;
    }

    disconnect(this, &MainWindow::inputFinish, this, &MainWindow::calc);
    // 使用 QTextStream 写入数据
    QTextStream out(&file);

    QEventLoop loop; // 事件循环

    getPoint();

    // 连接自定义信号到退出事件循环
    connect(this, &MainWindow::inputFinish, &loop, &QEventLoop::quit);

    // 启动事件循环，直到收到信号才会继续
    loop.exec();

    out << points.size() << endl;
    for(auto point : points){
        double x = point.first, y = point.second;
        out << QString("%1 %2").arg(x, 0, 'g', 30).arg(y, 0, 'g', 30) << endl;
    }

    // 关闭文件
    file.close();
    connect(this, &MainWindow::inputFinish, this, &MainWindow::calc);

    QMessageBox::information(this, "OK!", "file has been saved to " + filePath);
}
