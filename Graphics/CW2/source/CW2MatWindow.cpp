#include "CW2MatWindow.h"
#include "CW2Widget.h"
// #include <QWidget>

// Simple constructor to init, and arrange the elements in the widget
CW2MatWindow::CW2MatWindow(QWidget *parent) : QWidget(parent) {
    setWindowFlags(Qt::SubWindow);

    winLayout = new QGridLayout(this);
    a1 = new QLineEdit("0",this); 
    a2 = new QLineEdit("0",this); 
    a3 = new QLineEdit("0",this); 
    a4 = new QLineEdit("0",this); 
    d1 = new QLineEdit("0",this); 
    d2 = new QLineEdit("0",this); 
    d3 = new QLineEdit("0",this); 
    d4 = new QLineEdit("0",this); 
    sp1 = new QLineEdit("0",this); 
    sp2 = new QLineEdit("0",this); 
    sp3 = new QLineEdit("0",this); 
    sp4 = new QLineEdit("0",this); 
    sh = new QLineEdit("0",this); 
    confirm = new QPushButton("Confirm",this); 

    winLayout->addWidget(a1,0,0);
    winLayout->addWidget(a2,0,1);
    winLayout->addWidget(a3,0,2);
    winLayout->addWidget(a4,0,3);
    winLayout->addWidget(d1,1,0);
    winLayout->addWidget(d2,1,1);
    winLayout->addWidget(d3,1,2);
    winLayout->addWidget(d4,1,3);
    winLayout->addWidget(sp1,2,0);
    winLayout->addWidget(sp2,2,1);
    winLayout->addWidget(sp3,2,2);
    winLayout->addWidget(sp4,2,3);
    winLayout->addWidget(sh,3,0);
    winLayout->addWidget(confirm,4,3);

    // Connect the button signal to the custom slot
    connect(confirm, SIGNAL(clicked()), this, SLOT(callReturn()));

}

// Destructor, clean up Qt elements
CW2MatWindow::~CW2MatWindow() {
    delete a1;
    delete a2;
    delete a3;
    delete a4;
    delete d1;
    delete d2;
    delete d3;
    delete d4;
    delete sp1;
    delete sp2;
    delete sp3;
    delete sp4;
    delete sh;
    delete confirm;
    delete winLayout;
}

void CW2MatWindow::ResetInterface() {
    update();
}

// Custom slot to recieve the button press signal
// emits the retMaterials signal
void CW2MatWindow::callReturn() {
    GLfloat outputVars[3][4];
    GLfloat outputShin;

    outputVars[0][0] = a1->text().toFloat();
    outputVars[0][1] = a2->text().toFloat();
    outputVars[0][2] = a3->text().toFloat();
    outputVars[0][3] = a4->text().toFloat();
    outputVars[1][0] = d1->text().toFloat();
    outputVars[1][1] = d2->text().toFloat();
    outputVars[1][2] = d3->text().toFloat();
    outputVars[1][3] = d4->text().toFloat();
    outputVars[2][0] = sp1->text().toFloat();
    outputVars[2][1] = sp2->text().toFloat();
    outputVars[2][2] = sp3->text().toFloat();
    outputVars[2][3] = sp4->text().toFloat();
    outputShin = sh->text().toFloat();

    emit retMaterial(outputVars,outputShin);
    this->hide();
}

// Override of the show function
// Also displays the given material in the text boxes
void CW2MatWindow::show(matStruct* mat) {
    a1->setText(QString::number(mat->ambient[0]));
    a2->setText(QString::number(mat->ambient[1]));
    a3->setText(QString::number(mat->ambient[2]));
    a4->setText(QString::number(mat->ambient[3]));
    d1->setText(QString::number(mat->diffuse[0]));
    d2->setText(QString::number(mat->diffuse[1]));
    d3->setText(QString::number(mat->diffuse[2]));
    d4->setText(QString::number(mat->diffuse[3]));
    sp1->setText(QString::number(mat->specular[0]));
    sp2->setText(QString::number(mat->specular[1]));
    sp3->setText(QString::number(mat->specular[2]));
    sp4->setText(QString::number(mat->specular[3]));
    sh->setText(QString::number(mat->shininess));
    QWidget::show();
}