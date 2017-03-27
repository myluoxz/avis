#ifndef ENGINE_H
#define ENGINE_H


/*
 Engine functions used by actual game code
*/

#include <gl/glew.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Windows.h>
#include "Defines.h"

using namespace std;

#define rad2deg 57.2958f
#define deg2rad 0.0174533f
#define char0 (char)0

#define Lerp(a, b, c) (a*(1-c) + b*c)
#define Normalize(a) glm::normalize(a)
#define Distance(a, b) glm::distance(a, b)

typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;
typedef glm::vec2 Vec2;
typedef glm::vec3 Vec3;
typedef glm::vec4 Vec4;
typedef glm::quat Quat;

string to_string(Vec2 v), to_string(Vec3 v), to_string(Vec4 v), to_string(Quat v);
//Vec2 normalize

class Color {
public:
	Color() : r(0), g(0), b(0), a(0) {}
	Color(Vec4 v) : r((byte)round(v.r * 255)), g((byte)round(v.g * 255)), b((byte)round(v.b * 255)), a((byte)round(v.a * 255)) {}
	
	byte r, g, b, a;

	string hex() {
		string chs("0123456789ABCDEF");
		return "#" + chs[(r & 0xf0) >> 4] + chs[r & 0x0f] + chs[(g & 0xf0) >> 4] + chs[g & 0x0f] + chs[(b & 0xf0) >> 4] + chs[b & 0x0f] + chs[(a & 0xf0) >> 4] + chs[a & 0x0f];
	}

	Vec4 vec4() {
		return Vec4(r, g, b, a);
	}

	void SetHex(string h) {

	}
};

class Rect {
public:
	Rect(): x(0), y(0), w(1), h(1) {}
	Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}
	float x, y, w, h;

	bool Inside(Vec2 v);
};

class Random {
public:
	static float Value(), Range(float min, float max);
};

#define MOUSE_DOWN 0x01
#define MOUSE_HOLD 0x02
#define MOUSE_UP 0x03
#define MOUSE_HOVER_FLAG 0x10
#define MOUSE_CLICK 0x11 //use for buttons
#define MOUSE_PRESS 0x12 //use for buttons
#define MOUSE_RELEASE 0x13 //use for buttons

typedef unsigned char ALIGNMENT;
#define ALIGN_BOTLEFT 0x00
#define ALIGN_BOTCENTER 0x01
#define ALIGN_BOTRIGHT 0x02
#define ALIGN_MIDLEFT 0x10
#define ALIGN_MIDCENTER 0x11
#define ALIGN_MIDRIGHT 0x12
#define ALIGN_TOPLEFT 0x20
#define ALIGN_TOPCENTER 0x21
#define ALIGN_TOPRIGHT 0x22

typedef unsigned char ORIENTATION;
#define ORIENT_NONE 0x00
#define ORIENT_HORIZONTAL 0x01
#define ORIENT_VERTICAL 0x02

long long milliseconds();

class Editor;
class EditorBlock;
class EB_Inspector;

typedef unsigned char ASSETTYPE;
typedef int ASSETID;

//shorthands
Vec4 red(), green(), blue(), cyan(), yellow(), black(), white();
Vec4 red(float f), green(float f), blue(float f), cyan(float f), yellow(float f), black(float f), white(float f);
Vec4 red(float f, float i), green(float f, float i), blue(float f, float i), cyan(float f, float i), yellow(float f, float i), white(float f, float i);
Vec4 LerpVec4(Vec4 a, Vec4 b, float f);
float clamp(float f, float a, float b);
float repeat(float f, float a, float b);
Vec3 rotate(Vec3 v, Quat q);
glm::mat4 Quat2Mat(Quat q);
void _StreamWrite(const void* val, ofstream* stream, int size);
void _StreamWriteAsset(Editor* e, ofstream* stream, ASSETTYPE t, ASSETID i);
//void _Strm2Int(ifstream& strm, int& i), _Strm2Float(ifstream& strm, float& f), _Strm2Short(ifstream& strm, short& i);

template<typename T> void _Strm2Val(ifstream& strm, T &val) {
	/*
	long long pos = strm.tellg();
	byte size = sizeof(T);
	char c[8];
	strm.read(c, size);
	if (strm.fail()) {
		Debug::Error("Strm2Val", "Fail bit raised! (probably eof reached) " + to_string(pos));
	}
	//int rr(*(T*)c);
	val = *(T*)c;
	*/
	long long pos = strm.tellg();
	strm.read((char*)&val, (byte)sizeof(T));
	if (strm.fail()) {
		Debug::Error("Strm2Val", "Fail bit raised! (probably eof reached) " + to_string(pos));
	}
}
ASSETID _Strm2H(ifstream& strm);

string _Strm2Asset(ifstream& strm, Editor* e, ASSETTYPE& t, ASSETID& i, int maxL = 100);

//void* __GetCacheE(ASSETTYPE t, ASSETID i);
template<typename T> T* _GetCache(ASSETTYPE t, ASSETID i) {
#ifdef IS_EDITOR
	return static_cast<T*>(Editor::instance->GetCache(t, i));
#else
	return static_cast<T*>(AssetManager::GetCache(t, i));
#endif
}
float* hdr2float(byte imagergbe[], int w, int h);

//see SceneObjects.h
class Object;
class Component;
class Transform;
class Camera;
class MeshFilter;
class MeshRenderer;

//see Assetmanager
class AssetItem;
class AssetManager;

#define ASSETTYPE_UNDEF 0x00

#define ASSETTYPE_SCENE 0xf0
#define ASSETTYPE_TEXTURE 0x01
#define ASSETTYPE_HDRI 0x02
#define ASSETTYPE_RENDERTEXTURE 0x03
#define ASSETTYPE_SHADER 0x05
#define ASSETTYPE_MATERIAL 0x10
#define ASSETTYPE_BLEND 0x20
#define ASSETTYPE_MESH 0x21
#define ASSETTYPE_SCRIPT_H 0xfe
#define ASSETTYPE_SCRIPT_CPP 0xff
class AssetObject;
class Mesh;
class Texture;
class Background;
class SceneScript;
class SceneObject;

class Int2 {
public:	
	Int2(): x(0), y(0){}
	Int2(int x, int y): x(x), y(y) {}

	int x, y;

	bool operator==(const Int2& rhs)
	{
		return (x == rhs.x) && (y == rhs.y);
	}
};

class Debug {
public:
	static void Message(string c, string s);
	static void Warning(string c, string s);
	static void Error(string c, string s);
	static void ObjectTree(vector<SceneObject*> o);

	friend int main(int argc, char **argv);
protected:
	static ofstream* stream;
	static void Init(string path);

private:
	static void DoDebugObjectTree(vector<SceneObject*> o, int i);
};

class EB_Browser_File;

class IO {
public:
	static vector<string> GetFiles(const string& path, string ext = "");
	static vector<EB_Browser_File> GetFilesE(Editor* e, const string& path);
	static void GetFolders(const string& path, vector<string>* names, bool hidden = false);
	static bool HasDirectory(LPCTSTR szPath);
	static bool HasFile(LPCTSTR szPath);
	static string ReadFile(const string& path);
	static vector<string> GetRegistryKeys(HKEY key);
};

class Font {
public:
	Font(const string& pathb, const string& paths = "", int size = 0);
	bool loaded;
	uint width, padding, height, chars;
	float w2h;
	uint width2, padding2, height2, chars2;
	uint gwidth(float s) { return (s > sizeToggle) ? width2 : width; }
	uint gheight(float s) { return (s > sizeToggle) ? height2 : height; }
	uint gpadding(float s) { return (s > sizeToggle) ? padding2 : padding; }
	uint gchars(float s) { return (s > sizeToggle) ? chars2 : chars; }
	float w2h2;
	GLuint pointer;
	GLuint pointer2;
	int sizeToggle;
	GLuint getpointer(float size);
	float gw2h(float s) { return (s > sizeToggle) ? w2h2 : w2h; }
	ALIGNMENT alignment;

	Font* Align(ALIGNMENT a);
};

enum InputKey {
	Key_None = 0x00,
	Key_Backspace = 0x08,
	Key_Tab = 0x09,
	Key_Enter = 0x0D,
	Key_Shift = 0x10,
	Key_Control = 0x11,
	Key_Alt = 0x12,
	Key_CapsLock = 0x14,
	Key_Escape = 0x1B,
	Key_Space = 0x20,
	Key_LeftArrow = 0x25, Key_UpArrow, Key_RightArrow, Key_DownArrow,
	Key_Delete = 0x2E, 
	Key_0 = 0x30, Key_1, Key_2, Key_3, Key_4, Key_5, Key_6, Key_7, Key_8, Key_9,
	Key_A = 0x41, Key_B, Key_C, Key_D, Key_E, Key_F, Key_G, Key_H, Key_I, Key_J, Key_K, Key_L, Key_M, Key_N, Key_O, Key_P, Key_Q, Key_R, Key_S, Key_T, Key_U, Key_V, Key_W, Key_X, Key_Y, Key_Z,
	Key_NumPad0 = 0x60, Key_NumPad1, Key_NumPad2, Key_NumPad3, Key_NumPad4, Key_NumPad5, Key_NumPad6, Key_NumPad7, Key_NumPad8, Key_NumPad9,
	Key_Plus = 0xBB, Key_Comma, Key_Minus, Key_Dot
};

class Input {
public:
	static Vec2 mousePos, mousePosRelative, mouseDownPos;
	static bool mouse0, mouse1, mouse2;
	static byte mouse0State, mouse1State, mouse2State;
	static string inputString;
	static void UpdateMouseNKeyboard();

	static bool KeyDown(InputKey key), KeyHold(InputKey key), KeyUp(InputKey key);

protected:
	static bool keyStatusOld[256], keyStatusNew[256];
private:
	Input(Input const &); //deliberately not defined
	Input& operator= (Input const &);
};

class Display {
public:
	static int width, height;
	static glm::mat3 uiMatrix;

	static void Resize(int x, int y, bool maximize = false);
};

class Object {
public:
	Object(string nm = "");
	ulong id;
	string name;

	virtual bool ReferencingObject(Object* o) { return false; }
};

class AssetObject : public Object {
protected:
	AssetObject(ASSETTYPE t) : type(t), Object(), _changed(false) {}
	virtual  ~AssetObject() {}

	const ASSETTYPE type = 0;

	friend class Editor;
protected:
	bool _changed;

	//virtual void Load() = 0;
};

typedef byte SHADER_VARTYPE;
#define SHADER_INT 0x00
#define SHADER_FLOAT 0x01
#define SHADER_VEC2 0x02
#define SHADER_VEC3 0x03
#define SHADER_SAMPLER 0x10
#define SHADER_MATRIX 0x20

class ShaderValue {
public:
	ShaderValue() : i(0), x(0), y(0), z(0), m() {}
	int i;
	float x, y, z;
	glm::quat m;
};

enum SHADER_TEST {
	SHADER_TEST_NEVER,
	SHADER_TEST_ALWAYS,
	SHADER_TEST_LESS,
	SHADER_TEST_LEQUAL,
	SHADER_TEST_EQUAL,
	SHADER_TEST_GEQUAL,
	SHADER_TEST_GREATER,
	SHADER_TEST_NOTEQUAL
};

enum SHADER_CULL {
	SHADER_CULL_BACK,
	SHADER_CULL_FRONT,
	SHADER_CULL_NONE
};

class ShaderTags {
public:
	ShaderTags(): zTest(SHADER_TEST_LEQUAL), aTest(SHADER_TEST_ALWAYS), culling(SHADER_CULL_BACK), zWrite() {}
	SHADER_TEST zTest, aTest;
	SHADER_CULL culling;
	bool zWrite;

	void ApplyGL();

	static void Reset();
};

class ShaderVariable {
public:
	//ShaderVariable() : type(0), name(""), val(), min(0), max(0) {}

	SHADER_VARTYPE type;
	string name;
	ShaderValue val;

	float min, max;
};

class ShaderBase : public AssetObject {
public:
	ShaderBase(string path);
	ShaderBase(ifstream& stream, uint offset);
	//ShaderBase(string vert, string frag);
	~ShaderBase() {
		glDeleteProgram(pointer);
		for (ShaderVariable* v : vars)
			delete(v);
	}

	bool loaded;
	GLuint pointer;
	ShaderTags tags;
	vector<ShaderVariable*> vars;

	//void Set(byte type, GLint id, void* val) { values[type][id] = val; }
	//void* Get(byte type, GLint id) { return values[type][id]; }
	
	//removes macros, insert include files
	static bool Parse(ifstream* text, string path);
	static bool LoadShader(GLenum shaderType, string source, GLuint& shader, string* err = nullptr);
};

class MatVal_Tex {
	friend class Material;
	friend class MeshRenderer;
	friend void EBI_DrawAss_Mat(Vec4 v, Editor* editor, EB_Inspector* b, float &off);
protected:
	MatVal_Tex() : id(-1), tex(nullptr) {}

	int id;
	Texture* tex;
};

class Material : AssetObject {
public:
	ShaderBase* shader;

	Material(void);
	Material(ShaderBase* shad);
	~Material();
	//values applied to program on drawing stage
	unordered_map<SHADER_VARTYPE, unordered_map <GLint, void*>> vals;
	unordered_map<SHADER_VARTYPE, vector<string>> valNames;
	//unordered_map<GLint, ShaderVariable> vals;
	void SetTexture(string name, Texture* texture);
	void SetTexture(GLint id, Texture* texture);
	void SetFloat(string name, float val);
	void SetFloat(GLint id, float val);
	void SetInt(string name, int val);
	void SetInt(GLint id, int val);

	friend class Editor;
	friend class EB_Browser;
	friend class EB_Inspector;
	friend class Mesh;
	friend class MeshRenderer;
	friend class Scene;
	friend class AssetManager;
	friend int main(int argc, char **argv);
	friend void EBI_DrawAss_Mat(Vec4 v, Editor* editor, EB_Inspector* b, float &off);
protected:
	Material(string s);
	Material(ifstream& stream, uint offset);
	void _ReloadParams();

	int _shader;
	vector<SHADER_VARTYPE> valOrders;
	vector<byte> valOrderIds;
	vector<byte> valOrderGLIds;

	void Load();
	void Save(string path);
	void ApplyGL(glm::mat4& matrix);
	void ResetVals();
};

class Time {
public:
	static long long startMillis;
	static long long millis;
	static double time;
	static float delta;
};

enum DrawTex_Scaling {
	DrawTex_Fit,
	DrawTex_Crop,
	DrawTex_Stretch
};

class Engine {
public:
	static void BeginStencil(float x, float y, float w, float h);
	static void EndStencil();
	static void DrawTexture(float x, float y, float w, float h, Texture* texture, DrawTex_Scaling scl = DrawTex_Stretch);
	static void DrawTexture(float x, float y, float w, float h, Texture* texture, float alpha, DrawTex_Scaling scl = DrawTex_Stretch);
	static void DrawTexture(float x, float y, float w, float h, Texture* texture, Vec4 tint, DrawTex_Scaling scl = DrawTex_Stretch);
	static void DrawLine(Vec2 v1, Vec2 v2, Vec4 col, float width);
	static void DrawLine(Vec3 v1, Vec3 v2, Vec4 col, float width);
	static void DrawLineW(Vec3 v1, Vec3 v2, Vec4 col, float width);
	static void Label(float x, float y, float s, string str, Font* texture);
	static void Label(float x, float y, float s, string str, Font* texture, Vec4 Vec4);
	static void Label(float x, float y, float s, string str, Font* texture, Vec4 Vec4, float maxw);
	static byte Button(float x, float y, float w, float h);
	static byte Button(float x, float y, float w, float h, Vec4 normalVec4);
	static byte Button(float x, float y, float w, float h, Vec4 normalVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4);
	static byte Button(float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4);
	static byte Button(float x, float y, float w, float h, Texture* texture, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4, float uvx = 0, float uvy = 0, float uvw = 1, float uvh = 1);
	static byte Button(float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4);
	static byte EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4);
	static byte EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4);
	static byte EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4);
	static byte EButton(bool a, float x, float y, float w, float h, Texture* texture, Vec4 Vec4);
	static byte EButton(bool a, float x, float y, float w, float h, Texture* texture, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4);
	static byte EButton(bool a, float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4);
	static bool DrawToggle(float x, float y, float s, Vec4 col, bool t);
	static bool DrawToggle(float x, float y, float s, Texture* texture, bool t, Vec4 col=white(), ORIENTATION o = 0x00);
	static float DrawSliderFill(float x, float y, float w, float h, float min, float max, float val, Vec4 background, Vec4 foreground);
	//scaleType: 0=scale, 1=clip, 2=tile
	static void DrawProgressBar(float x, float y, float w, float h, float progress, Vec4 background, Texture* foreground, Vec4 tint, int padding, byte scaleType);
	static void DrawSky(Background* sky, float forX, float forY, float angleW, float rot);
	static void DrawSky(float x, float y, float w, float h, Background* sky, float forX, float forY, float angleW, float rot);

	static void RotateUI(float a, Vec2 point);
	static void ResetUIMatrix();

	static uint unlitProgram, unlitProgramA, unlitProgramC, skyProgram;
	static Font* defaultFont;
	
	static ulong GetNewId();

	static Texture* fallbackTex;

	static vector<ifstream> dataFiles;
	static unordered_map<ASSETTYPE, vector<pair<byte, long>>> dataPoss;
	static unordered_map<ASSETTYPE, vector<void*>> dataPossCache;

//private: //fk users
	static void DrawQuad(float x, float y, float w, float h, uint texture);
	static void DrawQuad(float x, float y, float w, float h, uint texture, Vec4 Vec4);
	static void DrawQuad(float x, float y, float w, float h, Vec4 Vec4);
	static void DrawQuad(float x, float y, float w, float h, uint texture, Vec2 uv0, Vec2 uv1, Vec2 uv2, Vec2 uv3, bool single, Vec4 Vec4);
	static void DrawCube(Vec3 pos, float dx, float dy, float dz, Vec4 Vec4);
	static void DrawIndices(const Vec3* poss, const int* is, int length, float r, float g, float b);
	static void DrawIndicesI(const Vec3* poss, const int* is, int length, float r, float g, float b);
	static ulong idCounter;
	static vector<Camera*> sceneCameras;
	
	static vector<ifstream*> assetStreams;
	static unordered_map<byte, vector<string>> assetData;
	//static unordered_map<string, byte[]> assetDataLoaded;
	//byte GetAsset(string name);
	friend int main(int argc, char **argv);
protected:
	static void Init(string path = "");
	static bool LoadDatas(string path);
};

class SceneSettings {
public:
	SceneSettings(): sky(nullptr), skyId(-1) {}
	
	Background* sky;
	Color ambientCol;

	friend class Editor;
	friend class EB_Inspector;
protected:
	int skyId;
};

class Scene {
public:
	Scene() : sceneName("newScene") {}
	Scene(ifstream& stream, long pos);
	~Scene() {}
	static Scene* active; //for use in-game
	static bool loaded() {
		return active != nullptr;
	}
	string sceneName;
	int sceneId;
	static void AddObject(SceneObject* object, SceneObject* parent = nullptr);
	static void DeleteObject(SceneObject* object);
	vector<SceneObject*> objects;
	SceneSettings settings;

	static void Load(uint i), Load(string name);

	friend int main(int argc, char **argv);
	friend class Editor;
	friend class AssetManager;
protected:
	static ifstream* strm;
	static vector<string> sceneNames;
	static vector<long> scenePoss;
	static void ReadD0();
	static void Unload();
	void Save(Editor* e);
};

class AssetManager {
	friend int main(int argc, char **argv);
	friend class Engine;
	friend class Scene;
	friend class SceneObject;
	friend class Material;
	template<typename T> friend T* _GetCache(ASSETTYPE t, ASSETID i);
	friend string _Strm2Asset(ifstream& strm, Editor* e, ASSETTYPE& t, ASSETID& i, int max);
protected:
	static unordered_map<ASSETTYPE, vector<string>> names;
	static unordered_map<ASSETTYPE, vector<pair<byte, uint>>> dataLocs;
	static unordered_map<ASSETTYPE, vector<void*>> dataCaches;
	static vector<ifstream*> streams;
	static void Init(string dpath);
	static void* CacheFromName(string nm);
	static ASSETID GetAssetId(string nm, ASSETTYPE &t);
	static void* GetCache(ASSETTYPE t, ASSETID i);
	static void* GenCache(ASSETTYPE t, ASSETID i);
};

#include "SceneObjects.h"

#endif