#ifndef __CW2_WINDOW_H__
#define __CW2_WINDOW_H__ 1

#include <QGLWidget>
#include <QBoxLayout>
#include "CW2Widget.h"

class CW2Window: public QWidget { 
	public:
        CW2Window(QWidget *parent);
        ~CW2Window();

        QBoxLayout *windowLayout;
        CW2Widget *cw2Widget;

        // resets all the interface elements
        void ResetInterface();
}; 
	
#endif
