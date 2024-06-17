#include "bvh.h"

#include <fstream>
#include <sstream>
#include <math.h>
#include <GL/glew.h>
#include <iostream>

#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/glm.hpp>

#include <Eigen/Dense>
#include <Eigen/QR>

using namespace std;

BVH::BVH() {
	motion = NULL;
	clear();
}

BVH::BVH(string _fileName) {
	motion = NULL;
	clear();

	Load(_fileName);
}

BVH::~BVH() {
	clear();
}

void BVH::clear() {
	unsigned int  i;
	for (i = 0; i < channels.size(); i++)
		delete  channels[i];
	for (i = 0; i < joints.size(); i++)
		delete  joints[i];
	if (motion != NULL)
		delete  motion;

	loaded = false;

	fileName = "";
	motionName = "";

	channelCount = 0;
	channels.clear();
	joints.clear();
	jointIndex.clear();

	frameCount = 0;
	interval = 0.0;
	motion = NULL;

}

void BVH::Load(string _fileName) {
	clear();

	size_t currLine = 0;
	
	ifstream file(_fileName);
	vector<string> fileCont;

	stringstream line;
	string token;
	vector < Joint*> jStack;
	Joint* currJ = NULL;
	Joint* newJ = NULL;
	bool isSite = false;
	glm::vec3 pos;

	if (!file.is_open()) {
		cout << "File could not open" << endl;
		return;
	}

	fileName = _fileName;

	size_t bSlashI = fileName.find_last_of('\\');
	if (bSlashI == string::npos) bSlashI = -1;
	size_t fSlashI = fileName.find_last_of('/');
	if (fSlashI == string::npos) fSlashI = -1;


	motionName = fileName.substr(bSlashI > fSlashI ? bSlashI + 1 : fSlashI + 1);
	motionName = motionName.substr(0, motionName.length() - 4);

	// Read file into vector
	string l;
	while (getline(file,l)) {
		fileCont.push_back(l);
	}
	// Parse heirarchy of file
	while (currLine < fileCont.size()) {
		line = stringstream(fileCont[currLine++]);

		// If no tokens on this line
		line >> token;

		if (token == "{") {
			jStack.push_back(currJ);
			currJ = newJ;
			continue;
		}

		if (token == "}") {
			currJ = jStack.back();
			jStack.pop_back();
			isSite = false;
			continue;
		}

		if (token == "ROOT" or token == "JOINT") {
			newJ = new Joint();
			newJ->index = joints.size();
			newJ->parent = currJ;
			newJ->hasSite = false;
			newJ->offset = glm::vec3(0.f, 0.f, 0.f);
			newJ->site = glm::vec3(0.f, 0.f, 0.f);
			newJ->selected = false;
			joints.push_back(newJ);

			if (currJ) currJ->children.push_back(newJ);

			line >> token;
			newJ->name = token;

			jointIndex[newJ->name] = newJ;
			continue;
		}

		if (token == "End") {
			newJ = currJ;
			isSite = true;
			continue;
		}

		if (token == "OFFSET") {

			if (currJ == NULL) { file.close(); return; };

			line >> token;
			pos.x = atof(token.c_str());
			line >> token;
			pos.y = atof(token.c_str());
			line >> token;
			pos.z = atof(token.c_str());

			if (isSite) {
				currJ->hasSite = true;
				currJ->site = pos;
			}
			else {
				currJ->offset = pos;
			}
			continue;
		}

		if (token == "CHANNELS") {
			line >> token;

			if (currJ == NULL) { file.close(); return; };
			
			currJ->channels.resize(atoi(token.c_str()));

			for (size_t i = 0; i < currJ->channels.size(); i++) {
				Channel* channel = new Channel();
				channel->joint = currJ;
				channel->index = channels.size();
				channels.push_back(channel);
				currJ->channels[i] = channel;

				line >> token;
				if (token == "Xrotation") channel->type = ChannelEnum::X_ROTATION;
				if (token == "Yrotation") channel->type = ChannelEnum::Y_ROTATION;
				if (token == "Zrotation") channel->type = ChannelEnum::Z_ROTATION;
				if (token == "Xposition") channel->type = ChannelEnum::X_POSITION;
				if (token == "Yposition") channel->type = ChannelEnum::Y_POSITION;
				if (token == "Zposition") channel->type = ChannelEnum::Z_POSITION;
			}
		}

		if (token == "MOTION") {
			break;
		}
	}


	// Parse frame count
	line = stringstream(fileCont[currLine++]);
	line >> token;
	if (token != "Frames:") { file.close(); return; };
	line >> token;
	frameCount = atoi(token.c_str());

	line = stringstream(fileCont[currLine++]);
	line >> token;
	if (token != "Frame") { file.close(); return; };
	line >> token;
	if (token != "Time:") { file.close(); return; };
	line >> token;
	interval = atof(token.c_str());

	channelCount = channels.size();
	motion = new double[frameCount * channelCount];


	// Parse motion
	// For each frame (line in the file)
	for (size_t i = 0; i < frameCount; i++) {
		line = stringstream(fileCont[currLine++]);
		line >> token;
		// Parse each channel's motion
		for (size_t j = 0; j < channelCount; j++) {
			motion[i * channelCount + j] = atof(token.c_str());
			line >> token;
		}
	}
	
	file.close();
	jPos.resize(joints.size());
	loaded = true;
}


void BVH::forwardKin(int frame, vector<glm::vec3>* posOut) {
	posOut->resize(joints.size());
	fkStack.empty();
	fkStack.push(glm::mat4x4(1.f));
	forwardKin(joints[0], motion + frame * channelCount, posOut);
}

void BVH::forwardKin(double* data, vector<glm::vec3>* posOut) {
	posOut->resize(joints.size());
	fkStack.empty();
	fkStack.push(glm::mat4x4(1.f));
	forwardKin(joints[0], data, posOut);
}

void BVH::forwardKin(Joint* j, double* data, vector<glm::vec3>* posOut) {
	fkStack.push(fkStack.top());
	
	// If j is root
	if (j->parent == NULL) {
		//cout << "Trans " << data[0]<<","<< data[1] << "," << data[2] << endl;
		fkStack.top() = glm::translate(fkStack.top(), glm::vec3(data[0],data[1],data[2]));
	}
	else {
		//cout << "Trans " << j->offset.x << "," << j->offset.y << "," << j->offset.z << endl;
		fkStack.top() = glm::translate(fkStack.top(), glm::vec3(j->offset.x, j->offset.y, j->offset.z));
	}

	// For each degree of freedom of the joint
	for (size_t i = 0; i < j->channels.size(); i++) {
		Channel* channel = j->channels[i];
		// Rotate according to the data
		switch (channel->type)
		{
		case ChannelEnum::X_ROTATION:
			//glRotatef((float)data[channel->index], 1.f, 0.f, 0.f);
			fkStack.top() = glm::rotate(fkStack.top(), glm::radians((float)data[channel->index]), glm::vec3(1.f, 0.f, 0.f));
			break;
		case ChannelEnum::Y_ROTATION:
			//glRotatef((float)data[channel->index], 0.f, 1.f, 0.f);
			fkStack.top() = glm::rotate(fkStack.top(), glm::radians((float)data[channel->index]), glm::vec3(0.f, 1.f, 0.f));
			break;
		case ChannelEnum::Z_ROTATION:
			//glRotatef((float)data[channel->index], 0.f, 0.f, 1.f);
			fkStack.top() = glm::rotate(fkStack.top(), glm::radians((float)data[channel->index]) , glm::vec3(0.f, 0.f, 1.f));
			break;
		default:
			break;
		}
	}

	posOut[0][j->index] = glm::vec3(fkStack.top()[3][0], fkStack.top()[3][1], fkStack.top()[3][2]);

	for (size_t i = 0; i < j->children.size(); i++)
	{
		forwardKin(j->children[i], data, posOut);
	}

	fkStack.pop();
}

void BVH::RenderJoints(vector<glm::vec3>* pos) {

	for (size_t i = 0; i < pos->size(); i++)
	{
		glm::vec3 p = pos->at(i);
		glPushMatrix();
		glTranslatef(p.x, p.y, p.z);

		Joint* j = joints[i];
		RenderJoint(j->selected);

		for (size_t i = 0; i < j->children.size(); i++) {
			RenderBone(p,pos->at(j->children[i]->index));
		}
		glPopMatrix();
	}
}

void BVH::RenderJoint(bool higlight) {
	GLUquadricObj* quadObj = gluNewQuadric();
	gluQuadricDrawStyle(quadObj, GLU_FILL);
	gluQuadricNormals(quadObj, GLU_SMOOTH);
	GLdouble radius = 0.1f;
	GLdouble slices = 8.;
	GLdouble stack = 8.;

	if (higlight) {
		glColor3f(1., 0., 0.);
	}
	else {
		glColor3f(0., 0., 1.);
	}


	gluSphere(quadObj, (GLdouble)radius * 2, slices, stack);
}

void BVH::RenderBone(glm::vec3 from, glm::vec3 to) {
	glPushMatrix();
	GLUquadricObj* quadObj = gluNewQuadric();
	gluQuadricDrawStyle(quadObj, GLU_FILL);
	gluQuadricNormals(quadObj, GLU_SMOOTH);
	GLdouble radius = 0.1f;
	GLdouble slices = 8.;
	GLdouble stack = 8.;

	glColor3f(0., 0., 0.);
	glm::vec3 offset = to - from;
	glm::vec3 offNorm = glm::normalize(offset);
	glm::vec3 cross = glm::cross(glm::vec3(0.,0.,1.), offNorm);
	float angle = glm::degrees(glm::orientedAngle(glm::vec3(0., 0., 1.), offNorm, cross));
	glRotatef(angle, cross.x, cross.y, cross.z);
	gluCylinder(quadObj, radius, radius, glm::length(offset), slices, stack);
	glPopMatrix();
}

void BVH::inverseKin(glm::vec3 targetP, int frame, int jointID) {
	Eigen::MatrixXd j = constructJac(frame, jointID);
	Eigen::MatrixXd jPseudo = j.completeOrthogonalDecomposition().pseudoInverse();

	vector<glm::vec3> jPosStart;

	forwardKin(motion + frame * channelCount, &jPosStart);

	Eigen::Vector3d p(targetP.x, targetP.y, targetP.z);
	Eigen::Vector3d jp(jPosStart[jointID].x, jPosStart[jointID].y, jPosStart[jointID].z);
	Eigen::Vector3d e = p-jp;

	Eigen::VectorXd deltaQ = jPseudo * e;

	//cout << deltaQ << endl;

	double* data = motion + frame * channelCount;
	for (size_t i = 0; i < deltaQ.rows(); i++)
	{
		data[i+3] += deltaQ(i);
	}

	vector<glm::vec3> jPosEnd;

	forwardKin(motion + frame * channelCount, &jPosEnd);
}

Eigen::MatrixXd BVH::constructJac(int frame, int jointID) {
	// Point to the start of the frame
	double* data = motion + frame * channelCount;

	Eigen::MatrixXd jacobian = Eigen::MatrixXd::Zero(3, channelCount - 3);
	vector<glm::vec3> oldJPos;
	forwardKin(data, &oldJPos);

	for (size_t i = 3; i < channelCount; i++)
	{
		vector<glm::vec3> newJPos;
		data[i] += 0.01;
		forwardKin(data, &newJPos);
		data[i] -= 0.01;
		glm::vec3 dJPos = newJPos[jointID] - oldJPos[jointID];
		jacobian(0, i - 3) = dJPos.x / 0.01;
		jacobian(1, i - 3) = dJPos.y / 0.01;
		jacobian(2, i - 3) = dJPos.z / 0.01;
	}
	//delete frameData;
	return jacobian;
}

void BVH::inverseKin(vector<glm::vec3> targetPs, int frame, vector<int> jointIDs) {
	Eigen::MatrixXd j = constructJac(frame, jointIDs);
	Eigen::MatrixXd jPseudo = j.completeOrthogonalDecomposition().pseudoInverse();

	vector<glm::vec3> jPosStart;

	forwardKin(motion + frame * channelCount, &jPosStart);

	Eigen::VectorXd tp = Eigen::VectorXd::Zero(3 * targetPs.size());
	Eigen::VectorXd op = Eigen::VectorXd::Zero(3 * targetPs.size());
	cout << "A" << endl;
	for (size_t i = 0; i < jointIDs.size(); i++) {
		cout << "a" << endl;
		tp[i * 3 + 0] = (double)targetPs[i].x;
		cout << "b" << endl;
		tp[i * 3 + 1] = (double)targetPs[i].y;
		cout << "c" << endl;
		tp[i * 3 + 2] = (double)targetPs[i].z;
		cout << "d" << endl;
		op[i * 3 + 0] = (double)jPosStart[i].x;
		cout << "e" << endl;
		op[i * 3 + 1] = (double)jPosStart[i].y;
		cout << "f" << endl;
		op[i * 3 + 2] = (double)jPosStart[i].z;
		cout << "g" << endl;
	}
	cout << "A" << endl;
	Eigen::VectorXd e = tp - op;

	Eigen::VectorXd deltaQ = jPseudo * e;

	//cout << deltaQ << endl;

	double* data = motion + frame * channelCount;
	for (size_t i = 0; i < deltaQ.rows(); i++)
	{
		data[i + 3] += deltaQ(i);
	}

	vector<glm::vec3> jPosEnd;

	forwardKin(motion + frame * channelCount, &jPosEnd);
}

Eigen::MatrixXd BVH::constructJac(int frame, vector<int> jointIDs) {
	// Point to the start of the frame
	double* data = motion + frame * channelCount;

	Eigen::MatrixXd jacobian = Eigen::MatrixXd::Zero(3 * jointIDs.size(), channelCount - 3);
	vector<glm::vec3> oldJPos;
	forwardKin(data, &oldJPos);

	for (size_t i = 3; i < channelCount; i++)
	{
		vector<glm::vec3> newJPos;
		data[i] += 0.0001;
		forwardKin(data, &newJPos);
		data[i] -= 0.0001;
		for (size_t j = 0; j < jointIDs.size(); j++)
		{
			glm::vec3 dJPos = newJPos[jointIDs[j]] - oldJPos[jointIDs[j]];
			jacobian(j * 3 + 0, i - 3) = dJPos.x / 0.0001;
			jacobian(j * 3 + 1, i - 3) = dJPos.y / 0.0001;
			jacobian(j * 3 + 2, i - 3) = dJPos.z / 0.0001;
		}
	}
	//delete frameData;
	return jacobian;
}