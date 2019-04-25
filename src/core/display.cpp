#include "ChokoLait.h"

int Display::width = 256, Display::height = 256;
int Display::actualWidth = 256, Display::actualHeight = 256;
int Display::frameWidth = 256, Display::frameHeight = 256;

float Display::dpiScl = 1;

NativeWindow* Display::window = nullptr;
CURSORTYPE Display::cursorType;

void Display::OnDpiChange() {
	ChokoLait::ReshapeGL(window, actualWidth, actualHeight);
	Scene::dirty = true;
	UI::font.ClearGlyphs();
	if (UI::font2) UI::font2.ClearGlyphs();
}

void Display::Resize(int x, int y, bool maximize) {
	glfwSetWindowSize(window, x, y);
}

void Display::SetCursor(CURSORTYPE type) {
	cursorType = type;
}