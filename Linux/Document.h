#pragma once
#include "../Engine/Namespace/RootNamespace.h"
#include "../Graphs/Plot.h"
#include <string>
#include "Undo.h"

struct Document
{
	Document() : plot(rns), need_redraw(true) { }

	virtual void load(const std::string &path);
	virtual void load_default();
	void clear();
	void save() const{ saveAs(path); }
	void saveAs(const std::string &path) const;

	void redraw(){ need_redraw = true; }
	void recalc(Plot &plot);
	void recalc(Graph *g);

	RootNamespace       rns;
	Plot                plot;
	mutable std::string path; // empty for new documents
	UndoTracker         ut;

	// Here follow undo-registering methods for every user-
	// triggerable modification!
	bool toggleAxis();
	bool toggleDisco();
	bool toggleClip();
	bool toggleClipCustom();
	bool toggleClipLock();
	bool setClipLock(bool lock, const P3f &normal);
	bool resetClipLock();
	bool setClipDistance(float v);
	bool setGrid(GridStyle v);
	bool toggleGrid();
	bool setQuality(double v);
	bool setGridDensity(double v);
	bool setHistoScale(double v);
	bool setDisplayMode(ShadingMode m);
	bool setVFMode(VectorfieldMode m);
	bool cycleVFMode(int d);
	bool setAxisGrid(AxisOptions::AxisGridMode m);
	bool setHistoMode(HistogramMode m);
	bool setMaskParam(double v);
	bool loadCustomMask(const std::string &path);
	bool setMask(MaskStyle m);
	bool setMask(GL_Mask &v); // leaves v with the previous value!
	//-------------------------------------------------------------
	bool setFog(double v);
	bool setLineWidth(double v);
	bool setShinyness(double v);
	bool setTextureStrength(double v);
	bool setReflectionStrength(double v);
	bool setAAMode(AntialiasMode m);
	bool setTransparencyMode(const GL_BlendMode &m);
	bool setTextureMode(TextureProjection m);
	bool setBgColor(const GL_Color &v);
	bool setAxisColor(const GL_Color &v);
	bool setFillColor(const GL_Color &v);
	bool setGridColor(const GL_Color &v);
	bool setTexture(GL_Image &v); // leaves v with the previous value!
	bool setTexture(GL_ImagePattern v);
	bool setReflectionTexture(GL_Image &v); // leaves v with the previous value!
	bool setReflectionTexture(GL_ImagePattern v);
	bool loadTexture(const std::string &path);
	bool loadReflectionTexture(const std::string &path);
	bool setAxisFont(const std::string &name, float size);
	//-------------------------------------------------------------
	bool setF1(const std::string &f) { return setF(1, f); }
	bool setF2(const std::string &f) { return setF(2, f); }
	bool setF3(const std::string &f) { return setF(3, f); }
	bool setF(int i, const std::string &s);
	bool setDomain(GraphType t);
	bool setCoords(GraphCoords c);
	bool setMode(GraphMode m);
	bool deleteGraph(IDCarrier::OID g_, IDCarrier::OID g_sel = 0);
	bool undeleteGraph(const std::vector<char> &data, IDCarrier::OID g_, bool make_current);
	bool addGraph();
	bool selectGraph(int i);
	bool toggleGraphVisibility();
	//-------------------------------------------------------------
	bool deleteParam(IDCarrier::OID g_);
	bool undeleteParam(const std::vector<char> &data, IDCarrier::OID g_);
	bool modifyParam(const std::vector<char> &data, IDCarrier::OID p_);
	//-------------------------------------------------------------
	bool deleteDef(IDCarrier::OID f_);
	bool undeleteDef(const std::vector<char> &data, IDCarrier::OID f_);
	bool modifyDef(const std::vector<char> &data, IDCarrier::OID f_);
	//-------------------------------------------------------------
	void undoForCam();
	bool setCam(const Quaternion &rot, double zoom);
	void undoForAxis();
	bool setAxis(const P3d &center, const P3d &range);
	void undoForInRange();
	bool setInRange(const P2d &center, const P2d &range);
	void undoForParam(Parameter *p);
	bool setParamValue(IDCarrier::OID p_, cnum value);

protected:
	bool need_redraw;
};

