#include "CW2Window.h"
#include "CW2Widget.h"

CW2Window::CW2Window(QWidget *parent) : QWidget(parent) {
	windowLayout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    windowLayout->setSpacing(0);
    windowLayout->setMargin(0);
    
    cw2Widget = new CW2Widget(this);
    windowLayout->addWidget(cw2Widget);
} 

CW2Window::~CW2Window() {
    delete cw2Widget;
	delete windowLayout;
}

void CW2Window::ResetInterface() {
    cw2Widget->update();
	update();
}
