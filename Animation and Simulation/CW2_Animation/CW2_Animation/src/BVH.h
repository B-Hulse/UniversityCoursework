#pragma once

#include <vector>
#include <stack>
#include <map>
#include <string>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <Eigen/Core>

using namespace std;

class BVH {
public: 
	// Enumeration of the channel types
	enum class ChannelEnum
	{
		X_ROTATION, Y_ROTATION, Z_ROTATION,
		X_POSITION, Y_POSITION, Z_POSITION
	};
	struct  Joint;

	// Single channel in the file
	struct  Channel
	{
		// Joint pointer associated with the channel
		Joint* joint;

		// The type of the channel
		ChannelEnum type;

		// Index in the channels vector of the channel
		int index;
	};

	// Single joint instance
	struct  Joint
	{
		// Name of the joint
		string name;
		// Index in the joints vecotr
		int                  index;

		Joint* parent;
		vector< Joint* > children;

		// Offset of the base of the joint
		glm::vec3 offset;

		// site refers to the end of a join with no children
		bool hasSite;
		// End point of the joint
		glm::vec3 site;

		// Pointers to the channels associated with the joint
		vector< Channel* > channels;

		bool selected;
	};

public:
	// Whether the file was loaded sucessfully
	bool loaded;

	// File info
	string fileName;
	string motionName;

	int channelCount;
	vector<Channel*> channels;
	vector<Joint*> joints;
	// The pointer to each joint by name
	map<string, Joint*> jointIndex;

	int frameCount;
	double interval;
	// Array of all motion values will be channelCount*frameCount long
	double* motion;

	// Store the world position after each frame of the joints
	vector<glm::vec3> jPos;

	stack<glm::mat4x4> fkStack;

public:
	BVH();
	BVH(string _fileName);
	~BVH();

	void clear();

	void Load(string _fileName);

	double getMotion(int frame, int chan) { return motion[frame * channelCount + chan]; }
	void setMotion(int frame, int chan, double val) { motion[frame * channelCount + chan] = val; }

public:
	void forwardKin(int frame, vector<glm::vec3>* posOut);
	void forwardKin(double* data, vector<glm::vec3>* posOut);
	void forwardKin(Joint* j, double * data, vector<glm::vec3>* posOut);
	void RenderJoints(vector<glm::vec3>* pos);
	void RenderJoint(bool highlight = false);
	void RenderBone(glm::vec3 from, glm::vec3 to);

	void inverseKin(glm::vec3 targetP, int frame, int jointID);
	Eigen::MatrixXd constructJac(int frame, int jointID);

	void inverseKin(vector<glm::vec3> targetPs, int frame, vector<int> jointIDs);
	Eigen::MatrixXd constructJac(int frame, vector<int> jointIDs);
};