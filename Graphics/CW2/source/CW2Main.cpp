#include <QApplication>
#include <QVBoxLayout>
#include "CW2Window.h"
#include <GL/glut.h>

#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);
    
    CW2Window *window = new CW2Window(NULL);

	glutInit(&argc,argv);

    window->resize(1000, 1000);
    if (argc > 1) {
        std::cout << argv[1] << '\n';
        if (std::string(argv[1]) == "-f") {
            window->showFullScreen();
        }
    }
    window->show();

	// start it running
	app.exec();

	// clean up
	//	delete controller;
	delete window;
	
	// return to caller
	return 0; 
}
