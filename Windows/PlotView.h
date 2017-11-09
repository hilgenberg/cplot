#pragma once
#include "afxwin.h"
#include "../Graphs/OpenGL/GL_RM.h"
#include "../Graphs/OpenGL/GL_StringCache.h"
#include "../Engine/Namespace/Parameter.h"
#include "../Utility/System.h"
class Document;

class PlotView : public CWnd
{
public:
	PlotView();
	~PlotView();

	COLORREF GetBgColor();
	void SetDoc(Document *d) { doc = d; }

	BOOL PreCreateWindow(CREATESTRUCT &cs);
	BOOL Create(const RECT &rect, CWnd *parent, UINT ID);
	int  OnCreate(LPCREATESTRUCT cs);
	void OnDestroy();

	BOOL OnEraseBkgnd(CDC *dc);
	void OnPaint();
	void OnSize(UINT type, int w, int h);

	void OnLButtonDown(UINT flags, CPoint p);
	void OnMButtonDown(UINT flags, CPoint p);
	void OnRButtonDown(UINT flags, CPoint p);
	void OnButtonDown (int i, CPoint p);
	void OnLButtonUp  (UINT flags, CPoint p);
	void OnMButtonUp  (UINT flags, CPoint p);
	void OnRButtonUp  (UINT flags, CPoint p);
	void OnButtonUp   (int i, CPoint p);
	void OnMouseMove  (UINT flags, CPoint p);
	BOOL OnMouseWheel (UINT flags, short dz, CPoint p);

	void OnKeyDown   (UINT c, UINT rep, UINT flags);
	void OnSysKeyDown(UINT c, UINT rep, UINT flags);
	void OnKeyUp     (UINT c, UINT rep, UINT flags);
	void OnSysKeyUp  (UINT c, UINT rep, UINT flags);
	UINT OnGetDlgCode();

	enum AnimType { Linear, Saw, PingPong, Sine };

private:
	void reshape();
	void zoom(double dy, int flags);
	void move(double dx, double dy, int flags);
	void handleArrows();
	bool animating();

	Document  *doc;
	CClientDC *dc;
	CRect      bounds;

	Timer    timer;
	double   tnf; // scheduled time for next frame
	double   last_frame; // time of last draw
	double   last_key; // time of last handleArrows
	GL_RM   *rm;

	union
	{
		struct
		{
			bool left : 1;
			bool right : 1;
			bool up : 1;
			bool down : 1;
			bool plus : 1;
			bool minus : 1;
		};
		char all;
	}
	arrows;

	double inertia[3]; // = {left/right, up/down, plus/minus}, all >= 0

	bool nums[10]; // keys to change params 0..9 (being '1',..'9','0') held down?
	short nums_on; // how many of them?

	struct ParameterAnimation
	{
		double dt, t0;
		cnum v0, v1;
		AnimType type;
		int reps; // -1 for forever
	};
	std::map<IDCarrier::OID, ParameterAnimation> panims;

	typedef UINT KeySym;
	std::map<KeySym, double> ikeys; // pressed key -> inertia
	std::set<KeySym> keys; // pressed keys

	CPoint m0; // last mouse position (for OnMouseMove)
	int    mb; // bits for LMR buttons

	DECLARE_DYNCREATE(PlotView)
	DECLARE_MESSAGE_MAP()
};
