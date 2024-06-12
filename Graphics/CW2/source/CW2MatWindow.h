#ifndef __CW2_MAT_H__
#define __CW2_MAT_H__ 1

#include <QWidget>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include "CW2Materials.h"

// A widget that is displayed as a window
// Allows the user to input material properties to change the torch light
class CW2MatWindow : public QWidget {
    Q_OBJECT
    public:
    CW2MatWindow(QWidget *parent);
    ~CW2MatWindow();
    void show(matStruct* mat);

    public slots:
    void callReturn();
    
    // Custom signal to send data to the main widget
    signals:
    void retMaterial(GLfloat vals[3][4], GLfloat shin);

    private:
    QGridLayout *winLayout;
    QLineEdit* a1;
    QLineEdit* a2;
    QLineEdit* a3;
    QLineEdit* a4;
    QLineEdit* d1;
    QLineEdit* d2;
    QLineEdit* d3;
    QLineEdit* d4;
    QLineEdit* sp1;
    QLineEdit* sp2;
    QLineEdit* sp3;
    QLineEdit* sp4;
    QLineEdit* sh;
    QPushButton* confirm;

    void ResetInterface();
};

#endif