#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "disp_window.h"
#include "trigrid_window.h"
#include "atten_trigrid_window.h"
#include "grid_window.h"
#include "fi2d_mesh.h"

using namespace std;

// Read the file and return the contents as a string
string readFile(string fileName) {
	ifstream file("./files/" + fileName);
	if (file.is_open()) {
		stringstream buf;
		buf << file.rdbuf() << endl;
		return buf.str();
		file.close();
	}
	else {
		cout << "File could not be opened." << endl;
		return "";
	}
}

// Write the new mesh to file
void writeFile(string fileName, fi2d_mesh mesh) {
	ofstream file("./files/edited_" + fileName);

	if (file.is_open()) {

		file << mesh.to2FI();

		file.close();
		cout << "File saved to " << "./files/edited_" + fileName << endl;
	}
	else {
		cout << "File could not be opened." << endl;
	}
}

fi2d_mesh displayMesh(fi2d_mesh mesh) {
	// Create a display window object
	disp_window win = disp_window();

	// load the mesh into the drawing window and display
	win.loadMesh(mesh);
	win.show();
	return win.saveMesh();
}

fi2d_mesh triGridDeformMesh(fi2d_mesh mesh) {
	// Create a display window object
	trigrid_window win = trigrid_window();

	string resp;
	cout << "How many internal vertices? (max 20): ";

	getline(cin, resp);

	int val = stoi(resp);

	if (val > 20)
		val = 20;
	win.internalVCount = stoi(resp);

	// load the mesh into the drawing window and display
	win.loadMesh(mesh);
	win.show();
	return win.saveMesh();
}

fi2d_mesh attenTriGridDeformMesh(fi2d_mesh mesh) {
	// Create a display window object
	atten_trigrid_window win = atten_trigrid_window();

	string resp;
	cout << "How many internal vertices? (max 20): ";

	getline(cin, resp);

	int val = stoi(resp);

	if (val > 20)
		val = 20;
	win.internalVCount = stoi(resp);

	// load the mesh into the drawing window and display
	win.loadMesh(mesh);
	win.show();
	return win.saveMesh();
}

fi2d_mesh gridDeformMesh(fi2d_mesh mesh) {
	// Create a display window object
	grid_window win = grid_window();

	// load the mesh into the drawing window and display
	win.loadMesh(mesh);
	win.show();
	return win.saveMesh();
}

void saveQuery(string fileName, fi2d_mesh mesh) {
	string saveResponse;
	cout << "Would you like to save that file? (yes/no): ";
	getline(cin, saveResponse);
	if (saveResponse == "yes") {
		writeFile(fileName, mesh);
	}
}


int main(void)
{
	while (1) {
		cout << "Please Specify a file to open (or type quit): ";

		// Get the user input
		string inStr;
		getline(cin, inStr);

		// If the user wants to quit, exit the loop
		if (inStr == "quit" or inStr == "") {
			return 1;
		}

		// Read the file with the filename supplied
		string fileContents;
		if ((fileContents = readFile(inStr)) != "") {

			// Create a mesh with the file contents
			fi2d_mesh mesh = fi2d_mesh();
			if (!mesh.init(fileContents)) {
				cout << "File not a valid 2fi file" << endl;
			}

			cout << "What would you like to do with the file (1/2/3/quit):" << endl << "	1 - Draw Mesh"<<endl<<"	2 - Grid Deform"<<endl<<"	3 - Trigrid Deform"<<endl<< "	4 - Attenuated Trigrid Deform" << endl;
			string inChoice;
			getline(cin, inChoice);
			if (inChoice == "quit" or inChoice == "") {
				return 1;
			}
			switch (stoi(inChoice)) {
			case 1:
				mesh = displayMesh(mesh);
				break;
			case 2:
			{
				mesh = gridDeformMesh(mesh);
				saveQuery(inStr, mesh);

				break;
			}
			case 3:
			{
				mesh = triGridDeformMesh(mesh);
				saveQuery(inStr, mesh);

				break;
			}
			case 4:
			{
				mesh = attenTriGridDeformMesh(mesh);
				saveQuery(inStr, mesh);

				break;
			}
			default:
				break;
			}
		}
	}
	return 1;
}
