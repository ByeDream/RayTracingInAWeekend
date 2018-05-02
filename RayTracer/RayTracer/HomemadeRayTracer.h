#pragma once

#include "Vec3.h"

class OutputImage;
class Hitable;
class Ray;
class InputListener;
class SimpleCamera;

class HomemadeRayTracer
{
public:
	HomemadeRayTracer(InputListener *inputListener, OutputImage *image);
	~HomemadeRayTracer();

	void						OnInit();
	void						OnUpdate();
	void						OnDestroy();
	void						HelpInfo();

	void						Render(OutputImage *image);

private:
	Vec3						Sample(const Ray &r, const Hitable &world) const;
	void						ConstructHitableWorld();
	void						DeconstructHitableWorld();

	InputListener *				m_inputListener{ nullptr };
	OutputImage *				m_image{ nullptr };
	Hitable *					m_hitableWorld{ nullptr };
	SimpleCamera *				m_camera{ nullptr };

	BOOL						m_enableNormalDisplay{ FALSE };
	BOOL						m_enblaeAA{ TRUE };
};
