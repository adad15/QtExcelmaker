#include "QtExcelmaker.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    // 创建 QApplication 对象管理整个应用程序
    QApplication app(argc, argv);
	// 创建并显示主窗口
    QtExcelmaker window;
    window.resize(800, 600);
    window.show();
	// 进入应用程序的主事件循环，循环接收信号，对信号做出处理。
    return app.exec();
}
