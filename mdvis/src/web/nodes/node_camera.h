#pragma once
#include "../annode.h"

class Node_Camera_Out : public AnNode {
public:
	static const string sig;
	Node_Camera_Out();
    
	void Execute() override;

	void OnSceneUpdate() override;
};