#include "node_tracetrj.h"
#include "md/Particles.h"

Node_TraceTrj::Node_TraceTrj() : AnNode(new DmScript(sig)) {
	title = "Trace Trajectory";
	titleCol = Vec3(0.3f, 0.5f, 0.3f);
    canTile = false;
	inputR.resize(1);
	script->invars.push_back(std::pair<string, string>("values", "list(2f)"));
}

void Node_TraceTrj::Execute() {
	has = false;
    if (!inputR[0].first) return;
	CVar& cv = inputR[0].first->conV[inputR[0].second];
	auto& sz = *cv.dimVals[0];
	if ((sz != Particles::anim.frameCount) || (*cv.dimVals[1] != 3)) return;
	pathSz = sz;
	path.resize(sz);
	memcpy(&path[0][0], *(float**)cv.value, sz * sizeof(Vec3));
	has = true;
}

void Node_TraceTrj::DrawScene() {
	if (!has || (pathSz != Particles::anim.frameCount) || pathSz == 1) return;

	auto fc = traceAll? Particles::anim.activeFrame + 1 : Particles::anim.frameCount;
	UI::SetVao(fc, &path[0][0]);
	
	auto mvp = MVP::projection() * MVP::modelview();

	glUseProgram(Engine::defProgramW);
	glUniformMatrix4fv(Engine::defWMVPLoc, 1, GL_FALSE, glm::value_ptr(mvp));
	glUniform4f(Engine::defWColLoc, col.r, col.g, col.b, col.a);
	glBindVertexArray(UI::_vao);
	glDrawArrays(GL_LINE_STRIP, 0, fc);
	glBindVertexArray(0);
	glUseProgram(0);
}

void Node_TraceTrj::LoadOut(const string& path) {
    Execute();
}