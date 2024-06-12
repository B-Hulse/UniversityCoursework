#include <QtGui>
#include <QApplication>
#include <QMainWindow>
#include <QMessageBox>
#include <QPainter>
#include <QLabel>
#include <QDesktopWidget>
#include "pixelwidget.hpp"

// I don't like the default way the main window pops up, I want it bigger
class MyMainWindow : public QMainWindow {

public: 
  
  MyMainWindow();

private:

};

MyMainWindow::MyMainWindow()
{
  // my window asks how big the desktop is; this is a widget for QT
  resize(QDesktopWidget().availableGeometry(this).size() * 0.7);
  // I've set my window to 70 % of the desktop  
}



int main(int argc, char **argv) {
  QApplication app(argc, argv);
  MyMainWindow window;

  // this is the canonical way of doing things: place widgets and draw on them
  // do not try to mess with MyMainWindow's 
  PixelWidget w(70,70);
  window.setCentralWidget(&w);
  window.show();

  return app.exec();
}
