#pragma once
#include "web/annode_internal.h"
#include "web/anweb.h"
#include "ui/popups.h"

class Node_Plot : public AnNode {
public:
	INODE_DEF_H

	Node_Plot();
	~Node_Plot();

	enum class TYPE : int {
		LINES,
		ALINES,
		SCATTER,
		DENSITY,
		CONTOUR
	} type;

	void DrawHeader(float& off) override;
	void DrawFooter(float& y) override;
	void Execute() override;
	void SaveOut(const std::string& path) override {}
	void LoadOut(const std::string& path) override;
	
	void OnConn(bool o, int i) override;

	void Save(XmlNode* n) override;
	void Load(XmlNode* n) override;

protected:
	bool useids;
	byte style;

	GLuint tex = 0;
	uint _w, _h;
	bool texDirty = false;

	std::vector<float> valXs;
	std::vector<std::vector<float>> valYs;
	std::vector<float*> _valYs;

	Popups::DropdownItem _di;

	void SetTex();
	void ExportCSV(const std::string& path);
};